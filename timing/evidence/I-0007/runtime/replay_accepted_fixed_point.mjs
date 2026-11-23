import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {ContinuousRecurrence} from './continuous_recurrence.mjs';
const here=path.dirname(fileURLToPath(import.meta.url)),out=path.join(here,'analysis/accepted-fixed-point');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex').toUpperCase();
const build=read(path.join(here,'transient-capture/ELAPSED-BUILD-VALIDATION.json'));
const executable=path.join(here,'transient-capture/bin/elapsed_recurrence_test.exe');
for(const a of build.artifacts.filter(a=>/elapsed_recurrence\.hpp$|elapsed_recurrence_test\.cpp$/.test(a.path)))assert.equal(hash(path.resolve(a.path)),a.sha256);
const identity=read(path.join(out,'native-build.json'));
assert.equal(hash(executable),identity.sha256);
fs.mkdirSync(out,{recursive:true});
const reports=[];
for(const [name,trials,label]of [
 ['20260919T184047Z-pid3804',[1],'accepted patched60 sprint control'],
 ['20260919T185120Z-pid3804',[1],'accepted V3 sprint165 with tail'],
 ['20260919T185858Z-pid3804',[1,2],'accepted roll-motion60/165']]) {
 const dir=path.join(here,'analysis',name),summary=read(path.join(dir,'causal-patch-summary.json'));
 assert(summary.timing_restored&&summary.exact_arithmetic_and_commit_checks);
 for(const [n,h]of Object.entries(summary.raw_sha256))assert.equal(hash(path.join(here,'sessions',name,n)),h);
 const windows=read(path.join(dir,'causal-patch-windows.json')).filter(r=>trials.includes(r.trial));
 assert(windows.length>0&&windows.every(r=>r.scale_after===1&&!r.reduced&&r.commit_exact));
 for(const trial of trials){
  const rows=windows.filter(r=>r.trial===trial),reference=summary.trials.find(t=>t.trial===trial);
  const dt=reference.median_delta;assert(dt>0&&dt<=.05);
  const p=new ContinuousRecurrence(1),q=new ContinuousRecurrence(1);
  const input=path.join(out,name+'-trial'+trial+'.csv'),nativeOut=input+'.native.txt';
  // Historical caller/gate absent: bound both possible nonreducing branches.
  fs.writeFileSync(input,rows.map(()=>`1,0,${dt}`).join('\n')+'\n');
  execFileSync(executable,[input,nativeOut]);
  const native=fs.readFileSync(nativeOut,'utf8').trim().split(/\r?\n/).map(Number);
  assert.equal(native.length,rows.length);
  rows.forEach((r,i)=>{assert.equal(p.advance(true,false,dt),1);assert.equal(q.advance(false,false,dt),1);assert.equal(native[i],1);});
  reports.push({session:name,label,trial,rows:rows.length,measured:rows.filter(r=>r.measured).length,source_window_sha256:hash(path.join(dir,'causal-patch-windows.json')),used_reference_delta:dt,changed_scalar_values:0});
 }
}
const result={reports,total_rows:reports.reduce((s,r)=>s+r.rows,0),native_test_sha256:identity.sha256,
 conclusion:'At scalar1, both inactive and recovering candidate branches preserve the accepted recorded state. Native implementation and independent JS agree for the retained accepted witnesses under the stated seed/branch conditions.',
 limits:'Conditional arithmetic noninterference only. Historical caller/gate/time identity and new hook behavior are not retroactively observed. Median delta is a replay parameter, not a newly proved clock. This does not replace live prototype acceptance or show immunity to other actions/states. Excluded old V2 trial2 because it has a known nonunit transient; not hidden as an accepted fixed-point witness.'};
fs.writeFileSync(path.join(out,'summary.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
