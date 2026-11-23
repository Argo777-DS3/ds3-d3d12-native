import assert from 'node:assert/strict';
import {analyze} from './analyze_capture.mjs';
const header=176,size=200,n=8,b=Buffer.alloc(header+n*size);
[0x493030384354524cn,200n,BigInt(n),10000000n,1n,1n,0n,0n,0x140000000n,2n,2n,2n,2n,1n,1n,0x50000000n,0x41f00000n,0n,0x43250000n,1n,0x41f00000n,0n].forEach((v,i)=>b.writeBigUInt64LE(v,i*8));
const u64={sequence:0,qpc:1,thread:2,site:3,object:4,context:5,control:6,out_ptr:7,loops_ptr:8,player:9,module:10,object_vtable:11,control_vtable:12,destination:13,caller:14};
const u32={valid:0,delta:1,central_delta:2,generator_time:3,control_current:4,speed:5,start:6,end:7,duration:8,mode:9,flags:10,loop_limit:11,forward_count:12,reverse_count:13,out_time:14,out_loops:15,id:16,ring_a:17,ring_b:18,ring_duration:19};
function row(i,site,values={}){const p=header+i*size;b.writeBigUInt64LE(BigInt(i),p);b.writeBigUInt64LE(BigInt(i+1),p+8);b.writeBigUInt64LE(7n,p+16);b.writeBigUInt64LE(site,p+24);b.writeUInt32LE(1,p+120);
  for(const[k,v]of Object.entries(values)){if(k in u64)b.writeBigUInt64LE(BigInt(v),p+u64[k]*8);else b.writeUInt32LE(v>>>0,p+120+u32[k]*4);}}
const delta=0x3bc696eb,player=0x51000000,module=0x52000000,object=0x53000000,control=0x54000000,out=0x55000000,loops=0x55000008,duration=0x40000000,speed=0x3f800000;
for(let chain=0;chain<2;chain++){const i=chain*4,a=chain?0x3ff00000:0x3f100000,c=chain?0x3f010101:0x3f120202;
  row(i,0x10768c0n,{object,context:0x56000000,control,player,module,object_vtable:0x142a0b9a8n,control_vtable:0x142a5d758n,delta,central_delta:delta,generator_time:a,control_current:a,speed,duration,mode:0,flags:0});
  row(i+1,0x1284d70n,{control,out_ptr:out,loops_ptr:loops,player,module,control_vtable:0x142a5d758n,delta,central_delta:delta,control_current:a,speed,duration,loop_limit:0xffffffff});
  row(i+2,0x1284ea8n,{control,out_ptr:out,loops_ptr:loops,player,module,control_vtable:0x142a5d758n,central_delta:delta,control_current:a,speed,duration,out_time:c,out_loops:chain});
  row(i+3,0x9b4f8dn,{player,module,destination:0x52000020+chain*16,central_delta:delta,id:0,ring_a:a,ring_b:c,ring_duration:duration,forward_count:chain});}
const ok=analyze(b);assert.equal(ok.bounded_capture_gate,true);assert.equal(ok.qualified_chains,2);assert.equal(ok.variable_central_delta_supported,true);
const mismatch=Buffer.from(b);mismatch.writeUInt32LE(0x3f999999,header+7*size+120+u32.ring_b*4);assert.equal(analyze(mismatch).bounded_capture_gate,false);
const restore=Buffer.from(b);restore.writeBigUInt64LE(0n,14*8);assert.equal(analyze(restore).bounded_capture_gate,false);
const fixed=Buffer.from(b);for(const i of [0,1,2,4,5,6]){fixed.writeUInt32LE(0x3c888889,header+i*size+120+u32.central_delta*4);if(i%4<2)fixed.writeUInt32LE(0x3c888889,header+i*size+120+u32.delta*4);}assert.equal(analyze(fixed).variable_central_delta_supported,false);
assert.throws(()=>analyze(b.subarray(0,b.length-1)));console.log('PASS: exact generator/control/output/ring chains; reject mismatch, failed restoration and truncation; distinguish variable delta from fixed 1/60.');
