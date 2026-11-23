import fs from 'node:fs';
import path from 'node:path';
import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import {fileURLToPath} from 'node:url';
const here=path.dirname(fileURLToPath(import.meta.url)),out=path.join(here,'analysis/elapsed-20260920-paired');
const json=p=>JSON.parse(fs.readFileSync(p,'utf8'));
const summary=json(path.join(out,'transient-summary.json'));
const pair=json(path.join(out,'elapsed-pair-evaluation.json'));
const rows=json(path.join(out,'transient-windows.json'));
const dir=path.join(here,'sessions',pair.session);
for(const [name,hash]of Object.entries(summary.raw_sha256))assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(dir,name))).digest('hex').toUpperCase(),hash);
const info=Object.fromEntries(fs.readFileSync(path.join(dir,'session-info.txt'),'utf8').trim().split(/\r?\n/).map(l=>{const i=l.indexOf('=');return[l.slice(0,i),l.slice(i+1)];}));
const [header,...lines]=fs.readFileSync(path.join(dir,'causal-distance-patch.csv'),'utf8').trim().split(/\r?\n/),keys=header.split(',');
const gates=new Map(lines.map(l=>Object.fromEntries(l.split(',').map((v,i)=>[keys[i],v]))).filter(r=>BigInt(r.rip_at_trap)-BigInt(info.image_base)===0x5c734cn).map(r=>[Number(r.broad_update_index),r]));
const reports=[];
const nonunitSurvey=[];
for(const trial of [1,2]) {
  const recovery=pair.reports[trial-1].recoveries[0];
  const segment=rows.filter(r=>r.trial===trial&&r.update>=recovery.first&&r.update<recovery.first+recovery.rows);
  assert.equal(segment.length,recovery.rows);
  const checks=segment.map(r=>{
    const raw=gates.get(r.update);assert(raw);
    const b=Buffer.from(raw.builder_bytes,'hex');
    const source=[0x1d0,0x1d4,0x1d8].map(o=>b.readFloatLE(o));
    const rate=b.readFloatLE(0x260),dt=r.gate_module_1c,expected=Math.min(1,r.gate_scale_before);
    assert(rate>0&&Number.isFinite(rate)&&dt>0&&source.every(Number.isFinite));
    const norm=Math.hypot(...source),ratio=norm/(rate*dt),error=ratio-expected;
    assert(Math.abs(error)<2e-7,'source-consumption mismatch');
    return {update:r.update,delta:dt,source,pre_factor_norm_rate:rate,expected_factor:expected,observed_factor:ratio,error,source_norm:norm};
  });
  reports.push({trial,rows:checks.length,max_factor_error:Math.max(...checks.map(r=>Math.abs(r.error))),
    pre_factor_rate_range:[Math.min(...checks.map(r=>r.pre_factor_norm_rate)),Math.max(...checks.map(r=>r.pre_factor_norm_rate))],
    source_norm_sum:checks.reduce((s,r)=>s+r.source_norm,0),
    source_norm_deficit:checks.reduce((s,r)=>s+(r.pre_factor_norm_rate*r.delta-r.source_norm),0),checks});
  const survey=rows.filter(r=>r.trial===trial&&r.gate_scale_before!==1).map(r=>{
    const b=Buffer.from(gates.get(r.update).builder_bytes,'hex');
    const v=[0x1d0,0x1d4,0x1d8].map(o=>b.readFloatLE(o)),rate=b.readFloatLE(0x260),dt=r.gate_module_1c;
    const norm=Math.hypot(...v),expected=Math.min(1,r.gate_scale_before);
    const usable=Number.isFinite(rate)&&rate>0&&dt>0&&Number.isFinite(norm);
    const ratio=usable?norm/(rate*dt):null;
    return {update:r.update,usable,source_norm:norm,rate,expected,ratio,matches:usable&&Math.abs(ratio-expected)<2e-7};
  });
  nonunitSurvey.push({trial,nonunit_entries:survey.length,usable:survey.filter(r=>r.usable).length,matches:survey.filter(r=>r.matches).length,
    exceptions:survey.filter(r=>!r.matches)});
}
const result={session:pair.session,reports,nonunit_survey:nonunitSurvey,qualified_source_rows:reports.reduce((s,r)=>s+r.rows,0),
  conclusion:'The recorded candidate recovery state reaches the nonzero source with the expected attenuation at60 and165. Together with the existing clock gate this is a bounded local causal validation, not just a scalar timer check.',
  limits:'Pre-factor rate is the existing builder+260 norm diagnostic, not a captured full pre-factor vector. Norm ratio tolerance reflects float32 normalization. Different source rates, animation phases and direction/input histories preclude equal world-path acceptance. Source-norm sums are not world displacement. No new live acquisition.'};
assert(nonunitSurvey.every(s=>s.matches===s.usable&&s.exceptions.every(r=>r.rate===0&&r.source_norm===0)));
fs.writeFileSync(path.join(out,'candidate-source-evaluation.json'),JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify({qualified_source_rows:result.qualified_source_rows,nonunit_survey:nonunitSurvey,reports:reports.map(({checks,...r})=>r),limits:result.limits},null,2));
