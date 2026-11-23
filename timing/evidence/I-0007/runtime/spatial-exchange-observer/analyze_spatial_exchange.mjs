import fs from 'node:fs';
import {pathToFileURL} from 'node:url';
import {validate} from './validate_event_sequence.mjs';

const u32x4=(row,prefix)=>[0,1,2,3].map(i=>Number.parseInt(row[`${prefix}${i}`].slice(2),16)>>>0);
function block(text){
  if(!/^[0-9A-Fa-f]{96}$/.test(text)) return [];
  return Array.from({length:12},(_,i)=>Number.parseInt(text.slice(i*8,i*8+8),16)>>>0);
}
function parseCsv(text){
  const lines=text.trim().split(/\r?\n/); if(lines.length<2) return [];
  const header=lines[0].split(',');
  return lines.slice(1).filter(Boolean).map(line=>Object.fromEntries(line.split(',').map((v,i)=>[header[i],v])));
}
function eventFromRow(row){
  const common={sequence:Number(row.seq),qpc:Number(row.qpc),tid:Number(row.tid),
    update_index:Number(row.broad_update_index),site:row.site,owner:row.owner,
    descriptor:row.descriptor,descriptor_base:row.descriptor_base,
    descriptor_offset:Number(row.descriptor_offset),selected:row.selected,
    pointer_chain_valid:row.pointer_chain_valid==='1',elapsed_u32x4:u32x4(row,'elapsed_b'),
    selected48:block(row.selected48_bits),selected48_valid:row.selected48_valid==='1'};
  if(row.site==='forward_write') return {...common,module28_object:row.module28_object,
    derived48:block(row.payload48_bits),derived48_valid:row.payload48_valid==='1'};
  if(row.site==='reverse_write') return {...common,current_transform48:block(row.payload48_bits),
    current_transform48_valid:row.payload48_valid==='1'};
  return common;
}

export function analyzeRows(rows,{requireBothDirections=true}={}){
  const overflow=rows.some(row=>row.overflow!=='0');
  const sites=Object.fromEntries(['forward_entry','forward_write','reverse_entry','reverse_write'].map(site=>
    [site,rows.filter(row=>row.site===site).length]));
  const pairing=validate(rows.map(eventFromRow));
  const direction_pairs={forward:pairing.accepted.filter(x=>x.direction==='forward').length,
    reverse:pairing.accepted.filter(x=>x.direction==='reverse').length};
  const structural_pass=!overflow&&pairing.rejected.length===0&&
    (!requireBothDirections||(direction_pairs.forward>0&&direction_pairs.reverse>0));
  return {rows:rows.length,sites,direction_pairs,rejected:pairing.rejected,overflow,structural_pass,
    limit:'Structural pairing only; no cross-rate equivalence or cadence claim.'};
}

if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
  const input=process.argv[2],output=process.argv[3];
  if(!input||!output) throw new Error('usage: node analyze_spatial_exchange.mjs <spatial-exchange.csv> <summary.json>');
  const summary=analyzeRows(parseCsv(fs.readFileSync(input,'utf8')));
  fs.writeFileSync(output,JSON.stringify(summary,null,2)+'\n');
  console.log(JSON.stringify(summary,null,2));
  if(!summary.structural_pass) process.exitCode=2;
}
