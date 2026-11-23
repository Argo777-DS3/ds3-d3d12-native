import fs from 'node:fs';
import {pathToFileURL} from 'node:url';

function parseCsv(text){
  const lines=text.trim().split(/\r?\n/); if(lines.length<2) return [];
  const header=lines.shift().split(',');
  return lines.filter(Boolean).map(line=>Object.fromEntries(
    line.split(',').map((value,index)=>[header[index],value])));
}

const sites=['forward_entry','forward_write','reverse_entry','reverse_write'];
const key=row=>[row.broad_update_index,row.owner,row.selected].join('|');
const lane=(block,index)=>block.slice(index*8,index*8+8);
const percentile=(values,p)=>{
  if(!values.length) return null;
  const sorted=[...values].sort((a,b)=>a-b);
  return sorted[Math.min(sorted.length-1,Math.floor((sorted.length-1)*p))];
};

export function analyzeDynamics(rows,qpcFrequency=10_000_000,
  interpretationLimit='Single target-60 trial. Ordering and bit equality are observed; gameplay semantics and cross-rate equivalence are not assigned.'){
  const groups=new Map();
  for(const row of rows){
    const id=key(row); if(!groups.has(id)) groups.set(id,{});
    groups.get(id)[row.site]=row;
  }
  const complete=[...groups.values()].filter(group=>sites.every(site=>group[site]));
  const selected=new Map(),latency={forward:[],reverse:[]};
  let strictOrder=0,forwardReverseSelectedEqual=0,forwardSelectedDerivedEqual=0;
  for(const group of complete){
    const ordered=sites.every((site,index)=>index===0||
      Number(group[sites[index-1]].seq)<Number(group[site].seq));
    if(ordered) ++strictOrder;
    if(group.forward_entry.selected48_bits===group.reverse_entry.selected48_bits)
      ++forwardReverseSelectedEqual;
    if(group.forward_entry.selected48_bits===group.forward_write.payload48_bits)
      ++forwardSelectedDerivedEqual;
    latency.forward.push((Number(group.forward_write.qpc)-Number(group.forward_entry.qpc))*1e6/qpcFrequency);
    latency.reverse.push((Number(group.reverse_write.qpc)-Number(group.reverse_entry.qpc))*1e6/qpcFrequency);
    const address=group.forward_entry.selected;
    if(!selected.has(address)) selected.set(address,{pairs:[],owners:new Set(),changed_lanes:Array(12).fill(0)});
    const item=selected.get(address),before=group.reverse_entry.selected48_bits,
      after=group.reverse_write.payload48_bits;
    item.pairs.push(group); item.owners.add(group.forward_entry.owner);
    for(let index=0;index<12;++index) if(lane(before,index)!==lane(after,index))
      ++item.changed_lanes[index];
  }
  const selectedSummary={};
  for(const [address,item] of selected){
    item.pairs.sort((a,b)=>Number(a.forward_entry.seq)-Number(b.forward_entry.seq));
    let reverseNoop=0,carryEqual=0;
    for(let index=0;index<item.pairs.length;++index){
      const group=item.pairs[index];
      if(group.reverse_entry.selected48_bits===group.reverse_write.payload48_bits) ++reverseNoop;
      if(index&&group.forward_entry.selected48_bits===item.pairs[index-1].reverse_write.payload48_bits)
        ++carryEqual;
    }
    selectedSummary[address]={
      pair_count:item.pairs.length,
      owner_count:item.owners.size,
      owners:[...item.owners],
      first_update:Number(item.pairs[0].forward_entry.broad_update_index),
      last_update:Number(item.pairs.at(-1).forward_entry.broad_update_index),
      reverse_noop_count:reverseNoop,
      reverse_changed_count:item.pairs.length-reverseNoop,
      reverse_changed_lane_counts:item.changed_lanes,
      previous_reverse_payload_equals_next_forward_selected:{
        equal:carryEqual,compared:Math.max(0,item.pairs.length-1),
      },
    };
  }
  const updateCounts=new Map();
  for(const row of rows) updateCounts.set(row.broad_update_index,(updateCounts.get(row.broad_update_index)??0)+1);
  const eventsPerUpdate={};
  for(const count of updateCounts.values()) eventsPerUpdate[count]=(eventsPerUpdate[count]??0)+1;
  const elapsedLane0={};
  const elapsedLane0BySelectedAndSite={};
  for(const row of rows.filter(row=>row.site.endsWith('_entry')))
  {
    elapsedLane0[row.elapsed_b0]=(elapsedLane0[row.elapsed_b0]??0)+1;
    const bucket=`${row.selected}|${row.site}`;
    elapsedLane0BySelectedAndSite[bucket]??={};
    elapsedLane0BySelectedAndSite[bucket][row.elapsed_b0]=
      (elapsedLane0BySelectedAndSite[bucket][row.elapsed_b0]??0)+1;
  }
  return {
    row_count:rows.length,
    complete_exchange_count:complete.length,
    strict_forward_then_reverse_count:strictOrder,
    forward_selected_equals_reverse_selected_count:forwardReverseSelectedEqual,
    forward_selected_equals_derived_count:forwardSelectedDerivedEqual,
    update_count:updateCounts.size,
    events_per_update_distribution:eventsPerUpdate,
    unique_owner_count:new Set(rows.map(row=>row.owner)).size,
    unique_selected_count:selected.size,
    unique_forward_derived_block_count:new Set(rows.filter(row=>row.site==='forward_write').map(row=>row.payload48_bits)).size,
    pair_latency_microseconds:{
      forward:{median:percentile(latency.forward,0.5),p95:percentile(latency.forward,0.95),max:Math.max(...latency.forward)},
      reverse:{median:percentile(latency.reverse,0.5),p95:percentile(latency.reverse,0.95),max:Math.max(...latency.reverse)},
    },
    entry_elapsed_lane0_bits:elapsedLane0,
    entry_elapsed_lane0_bits_by_selected_and_site:elapsedLane0BySelectedAndSite,
    selected_slots:selectedSummary,
    interpretation_limit:interpretationLimit,
  };
}

if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
  const input=process.argv[2],output=process.argv[3],interpretationLimit=process.argv[4];
  if(!input||!output) throw new Error('usage: node analyze_spatial_exchange_dynamics.mjs <spatial-exchange.csv> <output.json>');
  const result=analyzeDynamics(parseCsv(fs.readFileSync(input,'utf8')),10_000_000,
    interpretationLimit||undefined);
  fs.writeFileSync(output,JSON.stringify(result,null,2)+'\n');
  console.log(JSON.stringify(result,null,2));
}
