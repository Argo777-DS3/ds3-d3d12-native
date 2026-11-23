import fs from 'node:fs';

function parseCsv(file){
  const lines=fs.readFileSync(file,'utf8').trim().split(/\r?\n/);
  const header=lines.shift().split(',');
  return lines.filter(Boolean).map(line=>Object.fromEntries(
    line.split(',').map((value,index)=>[header[index],value])));
}
function floatFromBits(text){
  const buffer=new ArrayBuffer(4),view=new DataView(buffer);
  view.setUint32(0,Number.parseInt(text,16),true);
  return view.getFloat32(0,true);
}
function block(text){
  if(!text||text.length!==96) return null;
  return Array.from({length:12},(_,index)=>({
    bits:text.slice(index*8,index*8+8),
    value:floatFromBits(text.slice(index*8,index*8+8)),
  }));
}
const close=(left,right,tolerance=1e-5)=>Math.abs(left-right)<=tolerance;
function summarizeBlocks(blocks){
  const lanes=Array.from({length:12},(_,lane)=>{
    const values=blocks.map(item=>item[lane].value),bits=blocks.map(item=>item[lane].bits);
    let changes=0; for(let index=1;index<bits.length;++index) if(bits[index]!==bits[index-1]) ++changes;
    return {
      lane,sample_count:values.length,finite_count:values.filter(Number.isFinite).length,
      unique_bits_count:new Set(bits).size,consecutive_change_count:changes,
      minimum:Math.min(...values),maximum:Math.max(...values),
      constant_bits:new Set(bits).size===1?bits[0]:null,
    };
  });
  const quaternionNorms=blocks.map(item=>Math.hypot(...item.slice(4,8).map(lane=>lane.value)));
  return {
    sample_count:blocks.length,
    lanes,
    invariant_counts:{
      lane3_is_one:blocks.filter(item=>close(item[3].value,1)).length,
      lanes4_7_norm_is_one:quaternionNorms.filter(value=>close(value,1,1e-4)).length,
      lanes8_10_are_one:blocks.filter(item=>item.slice(8,11).every(lane=>close(lane.value,1))).length,
      lane11_is_one:blocks.filter(item=>close(item[11].value,1)).length,
    },
    lanes4_7_norm:{minimum:Math.min(...quaternionNorms),maximum:Math.max(...quaternionNorms)},
  };
}
function correlations(blocks,groups,updates){
  const fields=['before_x','before_z','before_y','after_x','after_z','after_y','before_angle','after_angle'];
  return Array.from({length:12},(_,lane)=>Object.fromEntries(fields.map(field=>{
    const errors=[];
    for(let index=0;index<blocks.length;++index){
      const update=updates.get(groups[index].forward_entry.broad_update_index);
      if(update) errors.push(Math.abs(blocks[index][lane].value-Number(update[field])));
    }
    return [field,{compared:errors.length,matches_1e_5:errors.filter(error=>error<=1e-5).length,
      maximum_absolute_error:errors.length?Math.max(...errors):null}];
  })));
}
function yawCorrelations(blocks,groups,updates){
  const result={};
  for(const field of ['before_angle','after_angle']){
    const errors=[];
    for(let index=0;index<blocks.length;++index){
      const update=updates.get(groups[index].forward_entry.broad_update_index);
      if(!update) continue;
      const yaw=2*Math.atan2(blocks[index][5].value,blocks[index][7].value);
      const difference=yaw-Number(update[field]);
      errors.push(Math.abs(Math.atan2(Math.sin(difference),Math.cos(difference))));
    }
    result[field]={compared:errors.length,matches_1e_5:errors.filter(error=>error<=1e-5).length,
      maximum_wrapped_absolute_error:errors.length?Math.max(...errors):null};
  }
  return result;
}
function distribution(values){
  if(!values.length) return {count:0,minimum:null,median:null,maximum:null};
  const sorted=[...values].sort((left,right)=>left-right);
  return {count:sorted.length,minimum:sorted[0],median:sorted[Math.floor(sorted.length/2)],maximum:sorted.at(-1)};
}
function rotateByQuaternion(vector,quaternion){
  const [x,y,z,w]=quaternion,[vx,vy,vz]=vector;
  const tx=2*(y*vz-z*vy),ty=2*(z*vx-x*vz),tz=2*(x*vy-y*vx);
  return [vx+w*tx+(y*tz-z*ty),vy+w*ty+(z*tx-x*tz),vz+w*tz+(x*ty-y*tx)];
}
function derivedTranslationRelation(groups,updates,threshold=1e-6,tolerance=2e-4){
  const activity={both_active:0,world_only:0,derived_only:0,neither:0};
  const activeErrors=[],normRatios=[],derivedNormPerDelta=[];
  for(const group of groups){
    const selected=block(group.forward_entry.selected48_bits);
    const current=block(group.reverse_write.payload48_bits);
    const derived=block(group.forward_write.payload48_bits);
    const world=current.slice(0,3).map((lane,index)=>lane.value-selected[index].value);
    const local=derived.slice(0,3).map(lane=>lane.value);
    const worldNorm=Math.hypot(...world),derivedNorm=Math.hypot(...local);
    const worldActive=worldNorm>threshold,derivedActive=derivedNorm>threshold;
    if(worldActive&&derivedActive) activity.both_active++;
    else if(worldActive) activity.world_only++;
    else if(derivedActive) activity.derived_only++;
    else activity.neither++;
    if(worldActive&&derivedActive){
      const quaternion=selected.slice(4,8).map(lane=>lane.value);
      const predicted=rotateByQuaternion(local,quaternion).map(value=>-value);
      activeErrors.push(Math.hypot(...predicted.map((value,index)=>value-world[index])));
      normRatios.push(derivedNorm/worldNorm);
      const update=updates.get(group.forward_entry.broad_update_index);
      if(update) derivedNormPerDelta.push(derivedNorm/Number(update.delta));
    }
  }
  return {
    activity_threshold:threshold,rotation_error_tolerance:tolerance,activity,
    active_row_count:activeErrors.length,
    negated_selected_quaternion_rotation_matches:activeErrors.filter(error=>error<=tolerance).length,
    rotation_error_norm:distribution(activeErrors),
    derived_to_world_norm_ratio:distribution(normRatios),
    derived_translation_norm_per_broad_delta:distribution(derivedNormPerDelta),
    formula:'reverse_current.translation - selected.translation ~= -rotate(selected.orientation, forward_derived.translation)',
  };
}

const [spatialFile,updatesFile,output,label='capture']=process.argv.slice(2);
if(!spatialFile||!updatesFile||!output)
  throw new Error('usage: node analyze_spatial_block_layout.mjs <spatial.csv> <updates.csv> <output.json> [label]');
const rows=parseCsv(spatialFile),updates=new Map(parseCsv(updatesFile).map(row=>[row.update_index,row]));
const groupsByKey=new Map();
for(const row of rows){
  const key=[row.broad_update_index,row.owner,row.selected].join('|');
  if(!groupsByKey.has(key)) groupsByKey.set(key,{});
  groupsByKey.get(key)[row.site]=row;
}
const sites=['forward_entry','forward_write','reverse_entry','reverse_write'];
const complete=[...groupsByKey.values()].filter(group=>sites.every(site=>group[site]));
const slotOwners=new Map(),slotCounts=new Map();
for(const group of complete){
  const slot=group.forward_entry.selected;
  if(!slotOwners.has(slot)) slotOwners.set(slot,new Set());
  slotOwners.get(slot).add(group.forward_entry.owner);
  slotCounts.set(slot,(slotCounts.get(slot)??0)+1);
}
const stableSlot=[...slotOwners].filter(([,owners])=>owners.size===1)
  .sort((left,right)=>slotCounts.get(right[0])-slotCounts.get(left[0]))[0]?.[0];
if(!stableSlot) throw new Error('no stable-owner slot');
const stableGroups=complete.filter(group=>group.forward_entry.selected===stableSlot)
  .sort((left,right)=>Number(left.forward_entry.broad_update_index)-Number(right.forward_entry.broad_update_index));
const roles={
  forward_selected:stableGroups.map(group=>block(group.forward_entry.selected48_bits)),
  forward_derived:stableGroups.map(group=>block(group.forward_write.payload48_bits)),
  reverse_selected:stableGroups.map(group=>block(group.reverse_entry.selected48_bits)),
  reverse_current:stableGroups.map(group=>block(group.reverse_write.payload48_bits)),
};
for(const [role,blocks] of Object.entries(roles)) if(blocks.some(item=>!item))
  throw new Error(`invalid block in ${role}`);
const result={
  schema:'I-0007-spatial-block-layout-v1',label,complete_exchange_count:complete.length,
  stable_selected_slot:stableSlot,stable_exchange_count:stableGroups.length,
  roles:Object.fromEntries(Object.entries(roles).map(([role,blocks])=>[role,{
    ...summarizeBlocks(blocks),
    update_correlations:correlations(blocks,stableGroups,updates),
    yaw_from_lanes5_7_correlations:yawCorrelations(blocks,stableGroups,updates),
  }])),
  derived_translation_relation:derivedTranslationRelation(stableGroups,updates),
  interpretation:'Tests a three-float position plus homogeneous lane, four-float unit orientation, and unit-scale candidate layout. Exact correlations and invariants are observations; field names remain provisional unless independently supported.',
  limit:'Stable-owner slot only. The analyzer does not establish a source type, coordinate convention, gameplay role or cross-rate equivalence.',
};
fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n');
console.log(JSON.stringify(result,null,2));
