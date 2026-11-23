import fs from 'node:fs';
import {pathToFileURL} from 'node:url';

function parseCsv(file){
  const lines=fs.readFileSync(file,'utf8').trim().split(/\r?\n/);
  const header=lines.shift().split(',');
  return lines.filter(Boolean).map(line=>Object.fromEntries(
    line.split(',').map((value,index)=>[header[index],value])));
}
function floatLane(block,index){
  const bytes=new ArrayBuffer(4),view=new DataView(bytes);
  view.setUint32(0,Number.parseInt(block.slice(index*8,index*8+8),16),true);
  return view.getFloat32(0,true);
}

export function correlate(spatial,updates,tolerance=1e-5,
  interpretation='Payload lanes 0 and 2 track the sampled player x/y fields in this target-60 trial. Lane 1 does not track sampled z. Correlation does not assign the full 48-byte block type or establish target-165 behavior.'){
  const groups=new Map();
  for(const row of spatial){
    const key=[row.broad_update_index,row.owner,row.selected].join('|');
    if(!groups.has(key)) groups.set(key,{});
    groups.get(key)[row.site]=row;
  }
  const complete=[...groups.values()].filter(group=>
    group.forward_entry&&group.forward_write&&group.reverse_entry&&group.reverse_write);
  const slotCounts=new Map();
  for(const group of complete){
    const slot=group.forward_entry.selected;
    if(!slotCounts.has(slot)) slotCounts.set(slot,{count:0,owners:new Set()});
    slotCounts.get(slot).count++;
    slotCounts.get(slot).owners.add(group.forward_entry.owner);
  }
  const stableSlot=[...slotCounts].filter(([,value])=>value.owners.size===1)
    .sort((left,right)=>right[1].count-left[1].count)[0]?.[0];
  if(!stableSlot) throw new Error('no stable-owner selected slot');
  const updatesByIndex=new Map(updates.map(row=>[row.update_index,row]));
  const samples=complete.filter(group=>group.forward_entry.selected===stableSlot)
    .sort((left,right)=>Number(left.forward_entry.broad_update_index)-Number(right.forward_entry.broad_update_index));
  const errors=[[],[],[]],matrix={changed_and_moving:0,changed_only:0,moving_only:0,neither:0};
  let previous=null,matched=0;
  for(const group of samples){
    const update=updatesByIndex.get(group.forward_entry.broad_update_index);
    if(!update) continue;
    ++matched;
    const payload=group.reverse_write.payload48_bits;
    const position=[Number(update.before_x),Number(update.before_z),Number(update.before_y)];
    for(let lane=0;lane<3;++lane) errors[lane].push(Math.abs(floatLane(payload,lane)-position[lane]));
    const changed=group.reverse_entry.selected48_bits!==payload;
    const moving=previous!==null&&Math.hypot(...position.map((value,index)=>value-previous[index]))>tolerance;
    if(changed&&moving) matrix.changed_and_moving++;
    else if(changed) matrix.changed_only++;
    else if(moving) matrix.moving_only++;
    else matrix.neither++;
    previous=position;
  }
  return {
    stable_selected_slot:stableSlot,
    matched_update_count:matched,
    comparison_tolerance:tolerance,
    reverse_payload_vs_player_snapshot:[0,1,2].map(lane=>({
      payload_lane:lane,
      snapshot_field:['before_x','before_z','before_y'][lane],
      matches:errors[lane].filter(error=>error<tolerance).length,
      maximum_absolute_error:Math.max(...errors[lane]),
    })),
    reverse_change_vs_consecutive_player_movement:matrix,
    interpretation,
  };
}

if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
  const [spatialFile,updatesFile,output,interpretation]=process.argv.slice(2);
  if(!spatialFile||!updatesFile||!output)
    throw new Error('usage: node analyze_spatial_player_correlation.mjs <spatial.csv> <updates.csv> <output.json>');
  const result=correlate(parseCsv(spatialFile),parseCsv(updatesFile),1e-5,
    interpretation||undefined);
  fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n');
  console.log(JSON.stringify(result,null,2));
}
