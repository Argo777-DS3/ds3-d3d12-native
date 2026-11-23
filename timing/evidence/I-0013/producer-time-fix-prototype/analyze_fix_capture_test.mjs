import assert from 'node:assert/strict';
import { parseCsv } from '../stage-boundary-observer/analyze_stage_boundary.mjs';
import { analyzeFix } from './analyze_fix_capture.mjs';
const h='delta_argument,central_delta,state_before,native_state,final_state,retention,step_ratio,mode,branch,correction_enabled,correction_applied,valid';
const dt=Math.fround(1/240),ref=Math.fround(0.0166666005),pre=Math.fround(.1),native=Math.fround(.2);
const final=Math.fround(Number(pre)+Number(Math.fround(native-pre))*(Number(dt)/Number(ref)));
const rows=parseCsv(`${h}\n${ref},${ref},0.1,0.2,0.2,0.6,1,1,1,1,0,1\n${dt},${dt},${pre},${native},${final},0.6,0.25,1,2,1,1,1\n`);
const result=analyzeFix(rows);assert.equal(result.structural_pass,true);assert.equal(result.exact_formula_rows,1);assert.equal(result.exact_native_passthrough_rows,1);
process.stdout.write(`${JSON.stringify({schema_version:1,status:'pass',cases:['native_passthrough','linear_dt_scaled_additive_step','float32_output'],limit:'Synthetic fix capture test only.'},null,2)}\n`);
