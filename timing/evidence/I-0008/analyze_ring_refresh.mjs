import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const repo = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../..");
const sessions = [
  { target: 60, id: "20260919T110038Z-pid27016" },
  { target: 90, id: "20260919T113730Z-pid23528" },
  { target: 120, id: "20260919T114725Z-pid27100" },
  { target: 165, id: "20260919T121245Z-pid6348" },
];

function parseCsv(file) {
  const lines = fs.readFileSync(file, "utf8").trim().split(/\r?\n/);
  const headers = lines[0].split(",");
  return lines.slice(1).map((line) => {
    const values = line.split(",");
    return Object.fromEntries(headers.map((header, index) => [header, values[index]]));
  });
}

function quantile(values, q) {
  if (!values.length) return null;
  const sorted = [...values].sort((a, b) => a - b);
  const index = (sorted.length - 1) * q;
  const lower = Math.floor(index);
  const upper = Math.ceil(index);
  if (lower === upper) return sorted[lower];
  return sorted[lower] + (sorted[upper] - sorted[lower]) * (index - lower);
}

function summarize(values) {
  return {
    count: values.length,
    min: values.length ? Math.min(...values) : null,
    median: quantile(values, 0.5),
    p95: quantile(values, 0.95),
    max: values.length ? Math.max(...values) : null,
  };
}

function keyOf(row) {
  return `${Number(row.animation_id)}|${Number(row.animation_max)}|${Number(row.play_speed)}`;
}

const results=[]; const exceptionRows=[];
for(const session of sessions) {
 const file=path.join(repo,'evidence/I-0005/runtime/pass-c/sessions',session.id,'semantic-state.csv');
 const rows=parseCsv(file); let lastChange=0; const gaps={}; const items=[]; let oldForward=0;
 for(let i=1;i<rows.length;i++) {
  const a=rows[i-1],b=rows[i];
  const changed=b.animation_time!==a.animation_time;
  if(!changed) continue;
  const j=lastChange;lastChange=i;
  if(b.animation_valid!=='1'||a.animation_valid!=='1'||Number(b.update_index)!==Number(a.update_index)+1||keyOf(b)!==keyOf(a)||Number(b.animation_time)<=Number(a.animation_time))continue;
  oldForward++;
  if(j===0) continue;
  const segment=rows.slice(j,i+1);
  if(segment.some((r,k)=>r.animation_valid!=='1'||keyOf(r)!==keyOf(b)||(k>0&&Number(r.update_index)!==Number(segment[k-1].update_index)+1)))continue;
  const gap=i-j;gaps[gap]=(gaps[gap]??0)+1;
  const inc=Number(b.animation_time)-Number(a.animation_time);
  const prevSum=rows.slice(j,i).reduce((s,r)=>s+Number(r.delta),0);
  const currentSum=rows.slice(j+1,i+1).reduce((s,r)=>s+Number(r.delta),0);
  const oldError=Math.abs(inc-10*Number(b.delta));
  const item={target:session.target,index:Number(b.update_index),gap,increment:inc,previous_sum:prevSum,current_sum:currentSum,previous_error:Math.abs(inc-prevSum),current_error:Math.abs(inc-currentSum),old_error:oldError,old_exception:Math.abs(inc/Number(b.delta)-10)>0.01};
  items.push(item); if(item.old_exception)exceptionRows.push(item);
 }
 results.push({target:session.target,session:session.id,original_forward_rows:oldForward,stable_refresh_intervals:items.length,gap_histogram:gaps,previous_sum_error:summarize(items.map(r=>r.previous_error)),current_sum_error:summarize(items.map(r=>r.current_error)),old_error:summarize(items.map(r=>r.old_error)),previous_sum_within_1e_5:items.filter(r=>r.previous_error<=1e-5).length,current_sum_within_1e_5:items.filter(r=>r.current_error<=1e-5).length,old_exceptions_retained:items.filter(r=>r.old_exception).length,old_exceptions_previous_sum_within_1e_5:items.filter(r=>r.old_exception&&r.previous_error<=1e-5).length,old_exceptions_current_sum_within_1e_5:items.filter(r=>r.old_exception&&r.current_error<=1e-5).length});
}
const out=path.join(repo,'evidence/I-0008/artifacts/retained-ring-audit');fs.mkdirSync(out,{recursive:true});
fs.writeFileSync(path.join(out,'ring-audit.json'),JSON.stringify({method:'Supplemental test prompted by static ten-slot ring. Original forward classification retained. Require entire last-refresh-to-current interval to preserve identity/max/speed and adjacent update indices. Compare increment to summed broad deltas, not ten times the current delta. Absolute 1e-5 gate is descriptive, not a causality test.',results,limits:'No ring cursor, writer invocation count or producer identity was captured. Ten-update refreshes support but do not prove one callback per update or identify the producing class. Sum phase is tested, not assumed.'},null,2)+'\n');
fs.writeFileSync(path.join(out,'exceptions.json'),JSON.stringify(exceptionRows,null,2)+'\n');console.log(JSON.stringify(results,null,2));
