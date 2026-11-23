import assert from 'node:assert/strict';
import fs from 'node:fs';
import {pathToFileURL} from 'node:url';

const directions={
  forward_entry:['forward','entry'],forward_write:['forward','write'],
  reverse_entry:['reverse','entry'],reverse_write:['reverse','write']
};
const key=e=>[directions[e.site]?.[0],e.tid,e.update_index,e.owner,e.selected].join('|');
const validCommon=e=>directions[e.site]&&Number.isInteger(e.sequence)&&e.sequence>0&&
  Number.isInteger(e.qpc)&&Number.isInteger(e.tid)&&Number.isInteger(e.update_index)&&
  Number.isInteger(e.descriptor_offset)&&e.pointer_chain_valid===true&&
  [e.owner,e.selected].every(v=>typeof v==='string'&&v.startsWith('0x'))&&
  (directions[e.site]?.[1]==='write'||
   [e.descriptor,e.descriptor_base].every(v=>typeof v==='string'&&v.startsWith('0x')));
const sameLanes=(a,b)=>Array.isArray(a)&&Array.isArray(b)&&a.length===4&&
  b.length===4&&a.every((v,i)=>Number.isInteger(v)&&v===b[i]);

export function validate(events){
  const open=new Map(),accepted=[],rejected=[];
  let previous=0;
  for(const event of events){
    const shape=directions[event.site];
    if(!validCommon(event)){rejected.push({sequence:event.sequence??null,reason:'invalid-common'});continue;}
    if(event.sequence<=previous){rejected.push({sequence:event.sequence,reason:'non-monotonic'});continue;}
    previous=event.sequence;
    const k=key(event),[direction,kind]=shape;
    if(kind==='entry'){
      if(open.has(k)){rejected.push({sequence:event.sequence,reason:'duplicate-entry'});continue;}
      if(!event.selected48_valid||!Array.isArray(event.selected48)||event.selected48.length!==12||
         !sameLanes(event.elapsed_u32x4,event.elapsed_u32x4)){
        rejected.push({sequence:event.sequence,reason:'invalid-entry-payload'});continue;
      }
      open.set(k,event); continue;
    }
    const entry=open.get(k);
    if(!entry){rejected.push({sequence:event.sequence,reason:'orphan-write'});continue;}
    const payloadOk=direction==='forward'
      ? typeof event.module28_object==='string'&&event.module28_object.startsWith('0x')&&
        event.derived48_valid&&Array.isArray(event.derived48)&&event.derived48.length===12&&sameLanes(entry.elapsed_u32x4,event.elapsed_u32x4)
      : event.current_transform48_valid&&Array.isArray(event.current_transform48)&&event.current_transform48.length===12;
    if(!payloadOk){rejected.push({sequence:event.sequence,reason:'invalid-write-payload'});open.delete(k);continue;}
    accepted.push({direction,entry_sequence:entry.sequence,write_sequence:event.sequence,key:k});
    open.delete(k);
  }
  for(const event of open.values()) rejected.push({sequence:event.sequence,reason:'orphan-entry'});
  return {accepted,rejected};
}

if(process.argv[1]&&import.meta.url===pathToFileURL(process.argv[1]).href){
const lanes=[1015580809,0,0,0],block=Array.from({length:12},(_,i)=>i);
const event=(sequence,site,extra={})=>({sequence,qpc:1000+sequence,tid:7,update_index:11,site,
  owner:'0x1000',descriptor:'0x2000',descriptor_base:'0x3000',descriptor_offset:64,
  selected:'0x3040',pointer_chain_valid:true,elapsed_u32x4:lanes,
  selected48:block,selected48_valid:true,...extra});
const cases={
  valid_forward:[event(1,'forward_entry'),event(2,'forward_write',{module28_object:'0x4000',derived48:block,derived48_valid:true})],
  valid_interleaved:[event(1,'forward_entry'),event(2,'reverse_entry'),
    event(3,'forward_write',{module28_object:'0x4000',derived48:block,derived48_valid:true}),
    event(4,'reverse_write',{current_transform48:block,current_transform48_valid:true})],
  mismatched_identity:[event(1,'forward_entry'),event(2,'forward_write',{selected:'0x9999',module28_object:'0x4000',derived48:block,derived48_valid:true})],
  elapsed_mismatch:[event(1,'forward_entry'),event(2,'forward_write',{module28_object:'0x4000',elapsed_u32x4:[1,0,0,0],derived48:block,derived48_valid:true})],
  orphan_write:[event(1,'reverse_write',{current_transform48:block,current_transform48_valid:true})],
  duplicate_entry:[event(1,'reverse_entry'),event(2,'reverse_entry')],
  non_monotonic:[event(2,'reverse_entry'),event(1,'reverse_write',{current_transform48:block,current_transform48_valid:true})]
};
const results=Object.fromEntries(Object.entries(cases).map(([name,events])=>[name,validate(events)]));
assert.equal(results.valid_forward.accepted.length,1);
assert.equal(results.valid_forward.rejected.length,0);
assert.equal(results.valid_interleaved.accepted.length,2);
assert.equal(results.valid_interleaved.rejected.length,0);
for(const name of ['mismatched_identity','elapsed_mismatch','orphan_write','duplicate_entry','non_monotonic']){
  assert.equal(results[name].accepted.length,0,name);
  assert.ok(results[name].rejected.length>0,name);
}
const summary={schema_version:1,cases:Object.fromEntries(Object.entries(results).map(([name,r])=>
  [name,{accepted:r.accepted.length,rejected:r.rejected.map(x=>x.reason)}])),status:'pass'};
fs.writeFileSync(new URL('pairing-validation.json',import.meta.url),JSON.stringify(summary,null,2)+'\n');
console.log(JSON.stringify(summary,null,2));
}
