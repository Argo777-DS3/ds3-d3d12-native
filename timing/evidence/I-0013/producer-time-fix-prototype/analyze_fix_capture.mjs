import fs from 'node:fs';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';

const f32=Math.fround;
const bits=value=>{const b=new ArrayBuffer(4);new Float32Array(b)[0]=f32(value);return new Uint32Array(b)[0];};
export function analyzeFix(rows){
  const required=['delta_argument','central_delta','state_before','native_state','final_state',
    'retention','step_ratio','mode','branch','correction_enabled','correction_applied','valid'];
  const missing=required.filter(key=>rows.length&&!(key in rows[0]));
  const valid=rows.filter(row=>row.valid==='1');
  const finite=valid.every(row=>['delta_argument','central_delta','state_before','native_state',
    'final_state','retention','step_ratio'].every(key=>Number.isFinite(Number(row[key]))));
  const branches=Object.fromEntries([...new Set(valid.map(row=>row.branch))].sort().map(branch=>
    [branch,valid.filter(row=>row.branch===branch).length]));
  let exactFormula=0, exactNative=0, applied=0;
  for(const row of valid){
    const pre=f32(Number(row.state_before)), native=f32(Number(row.native_state));
    const final=f32(Number(row.final_state)), delta=f32(Number(row.delta_argument));
    const ratio=Number(delta)/Number(f32(0.0166666005));
    if(row.correction_applied==='1'){
      applied++;
      let expected;
      if(row.branch==='3') expected=f32(Number(pre)*Math.pow(Number(f32(Number(row.retention))),ratio));
      else expected=f32(Number(pre)+Number(f32(native-pre))*ratio);
      expected=f32(Math.max(-0.5,Math.min(0.5,expected)));
      if(bits(expected)===bits(final)) exactFormula++;
    }else if(bits(native)===bits(final)) exactNative++;
  }
  const deltas=valid.map(row=>Number(row.delta_argument));
  return {structural_pass:rows.length>=2&&valid.length===rows.length&&missing.length===0&&finite&&
      exactFormula===applied&&exactNative===valid.length-applied,
    row_count:rows.length,valid_row_count:valid.length,missing_columns:missing,finite_values:finite,
    correction_enabled_rows:valid.filter(row=>row.correction_enabled==='1').length,
    correction_applied_rows:applied,exact_formula_rows:exactFormula,exact_native_passthrough_rows:exactNative,
    branches,delta:{min:Math.min(...deltas),max:Math.max(...deltas),mean:deltas.reduce((a,b)=>a+b,0)/deltas.length},
    limit:'Validates captured arithmetic and native passthrough only; visual terrain behavior requires operator observation.'};
}
if(process.argv[1]?.endsWith('analyze_fix_capture.mjs')){if(process.argv.length!==4)throw new Error('usage: node analyze_fix_capture.mjs INPUT.csv OUTPUT.json');const result=analyzeFix(parseCsv(fs.readFileSync(process.argv[2],'utf8')));fs.writeFileSync(process.argv[3],`${JSON.stringify(result,null,2)}\n`);if(!result.structural_pass)process.exitCode=2;}
