import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import {execFileSync} from 'node:child_process';
import {fileURLToPath} from 'node:url';
import {ContinuousRecurrence} from './continuous_recurrence.mjs';
const here=path.dirname(fileURLToPath(import.meta.url)),reports=[];
for(const hz of [60,165]){
  const dir=path.join(here,`analysis/transient-20260920-${hz}`);
  const rows=JSON.parse(fs.readFileSync(path.join(dir,'transient-windows.json'),'utf8'));
  const input=path.join(dir,'continuous-native-input.csv'),output=path.join(dir,'continuous-native-output.txt');
  fs.writeFileSync(input,rows.map(r=>`${+r.gate_active},${+!!r.reduced},${r.gate_module_1c}`).join('\n')+'\n');
  execFileSync(path.join(here,'transient-capture/bin/elapsed_recurrence_test.exe'),[input,output]);
  const actual=fs.readFileSync(output,'utf8').trim().split(/\r?\n/).map(Number),p=new ContinuousRecurrence(1);
  assert.equal(actual.length,rows.length);let changed=0;
  rows.forEach((r,i)=>{const predicted=p.advance(r.gate_active,!!r.reduced,r.gate_module_1c);assert.equal(Math.fround(actual[i]),predicted);if(predicted!==r.scale_after)changed++;});
  if(hz===60)assert.equal(changed,0);
  reports.push({hz,rows:rows.length,native_js_exact:actual.length,changed_from_recorded_native:changed,final:actual.at(-1)});
}
const report={classification:'Continuous native prototype and independent JS arithmetic agree; frozen-branch replay only',reports,live_tested:false,
 limits:'At a branch change the current interpolated value seeds a new reference segment. This is a candidate extension of native60 behavior, not uniquely proven design intent. Real altered movement can change future branches; live60 regression and165 closed-loop validation remain required.'};
fs.writeFileSync(path.join(here,'analysis/transient-20260920-165/continuous-native-validation.json'),JSON.stringify(report,null,2)+'\n');
console.log(JSON.stringify(report,null,2));
