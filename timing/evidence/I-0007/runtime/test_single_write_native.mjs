import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url)), out=path.join(here,'analysis/single-write');
const read=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex').toUpperCase();
const source=path.join(here,'analysis/elapsed-20260920-paired');
const summary=read(path.join(source,'transient-summary.json'));
for(const [n,h] of Object.entries(summary.raw_sha256))
  assert.equal(hash(path.join(here,'sessions/20260920T020120Z-pid2428',n)),h);
const rows=read(path.join(source,'transient-windows.json'));
const f=Math.fround,native=(s,a,r)=>!a?s:r?(s>.5?f(s*f(.8)):.5):(s<1?f(s*f(1.2)):1);
const executable=path.join(here,'transient-capture/bin/single_write_feedback_test.exe');
const reports=[];
for(const trial of [1,2]) {
  const selected=rows.filter(r=>r.trial===trial);
  const input=path.join(out,`native-trial${trial}.csv`), output=path.join(out,`native-trial${trial}-output.txt`);
  fs.writeFileSync(input,selected.map(r=>[r.update,r.gate_scale_before,+r.gate_active,+r.compare_seen,
    r.raw??0,r.governing_delta??0,r.gate_module_1c,
    native(r.gate_scale_before,r.gate_active,r.compare_seen&&r.raw<1)].join(',')).join('\n')+'\n');
  const log=execFileSync(executable,[input,output],{encoding:'utf8'});
  fs.writeFileSync(path.join(out,`native-trial${trial}-test-log.txt`),log);
  assert(log.includes('20 rejected transactions'));
  const actual=fs.readFileSync(output,'utf8').trim().split(/\r?\n/).map(Number);
  assert.equal(actual.length,selected.length);
  actual.forEach((v,i)=>assert.equal(f(v),selected[i].scale_after));
  reports.push({trial,rows:actual.length,exact_recorded_candidate_outputs:actual.length});
}
const artifacts=['transient-capture/single_write_feedback.hpp','transient-capture/single_write_feedback_test.cpp',
  'transient-capture/elapsed_recurrence.hpp','causal-patch/distance_normalization.hpp',
  'transient-capture/bin/single_write_feedback_test.exe','test_single_write_native.mjs'].map(p=>({path:p,sha256:hash(path.join(here,p))}));
const result={reports,artifacts,compiler:'MSVC Visual Studio18 Insiders',flags:'/O2 /MT /W4 /WX /EHsc /std:c++20',
  build_log:'native-build.txt',distinct_rejection_checks:20,
  limits:'Isolated transaction, not game instrumentation. Replay owner/epoch are synthetic identity1; historical owner identity is not requalified by this test. Native exit is counterfactual modeled input. Adapter must qualify caller, full observation coverage and original distance-frame matching, and stop a live trial on rejection. Acquisition DLLs unchanged. Concurrent intermediate visibility and full movement gates remain open.'};
fs.writeFileSync(path.join(out,'native-validation.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({reports,distinct_rejection_checks:20,limits:result.limits},null,2));
