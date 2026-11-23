import fs from 'node:fs';

function parseCsv(file) {
  const lines=fs.readFileSync(file,'utf8').trim().split(/\r?\n/);
  const header=lines.shift().split(',');
  return lines.filter(Boolean).map(line=>Object.fromEntries(
    line.split(',').map((value,index)=>[header[index],value])));
}

function floatFromBits(text) {
  const bytes=new ArrayBuffer(4),view=new DataView(bytes);
  view.setUint32(0,Number.parseInt(text.slice(2),16),true);
  return view.getFloat32(0,true);
}

function increment(map,key,field) {
  map[key]??={entries:0,matches_delta:0,matches_30x_delta:0,other:0};
  map[key].entries++;
  map[key][field]++;
}

const [spatialFile,updatesFile,output,label='capture']=process.argv.slice(2);
if(!spatialFile||!updatesFile||!output)
  throw new Error('usage: node analyze_spatial_elapsed_regimes.mjs <spatial.csv> <updates.csv> <output.json> [label]');

const spatial=parseCsv(spatialFile).filter(row=>row.site.endsWith('_entry'));
const updates=new Map(parseCsv(updatesFile).map(row=>[row.update_index,Number(row.delta)]));
const bySite={},bySelected={},byOwner={},rawBits={};
let matchesDelta=0,matches30=0,other=0,missingUpdate=0;
let maxDeltaError=0,max30Error=0;
for(const row of spatial) {
  const delta=updates.get(row.broad_update_index);
  if(delta===undefined) { missingUpdate++; continue; }
  const value=floatFromBits(row.elapsed_b0);
  const deltaError=Math.abs(value-delta),thirtyError=Math.abs(value-30*delta);
  let field='other';
  if(deltaError<=1e-6) {
    field='matches_delta'; matchesDelta++; maxDeltaError=Math.max(maxDeltaError,deltaError);
  } else if(thirtyError<=1e-5) {
    field='matches_30x_delta'; matches30++; max30Error=Math.max(max30Error,thirtyError);
  } else other++;
  increment(bySite,row.site,field);
  increment(bySelected,row.selected,field);
  increment(byOwner,row.owner,field);
  rawBits[row.elapsed_b0]=(rawBits[row.elapsed_b0]??0)+1;
}

const result={
  schema:'I-0007-spatial-elapsed-regimes-v1',
  label,
  entry_rows:spatial.length,
  matched_update_rows:spatial.length-missingUpdate,
  missing_update_rows:missingUpdate,
  classification:{
    matches_current_delta:matchesDelta,
    matches_30x_current_delta:matches30,
    other,
    maximum_delta_absolute_error:maxDeltaError,
    maximum_30x_delta_absolute_error:max30Error,
  },
  by_site:bySite,
  raw_lane0_bits:rawBits,
  by_selected:bySelected,
  by_owner:byOwner,
  limit:'Classifies recorded entry rows only. A rejected or overflowed capture remains rejected; row-level agreement does not establish accepted cross-rate equivalence or gameplay semantics.',
};
fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
