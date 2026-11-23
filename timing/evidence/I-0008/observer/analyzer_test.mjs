import assert from 'node:assert/strict';
import {analyze} from './analyze_capture.mjs';
const b=Buffer.alloc(72+21*280);
[0x4930303852494e47n,280n,21n,10000000n,1n,1n,0n,0n,0x140000000n].forEach((v,i)=>b.writeBigUInt64LE(v,i*8));
for(let i=0;i<21;i++){
  const p=72+i*280; b.writeBigUInt64LE(BigInt(i),p);b.writeBigUInt64LE(7n,p+16);b.writeUInt32LE(1,p+80);
  b.writeBigUInt64LE(i===0?0xF1B070n:(i%2?0x9B4F8Dn:0x9B4FC3n),p+24);
  if(i>0){let slot=Math.floor((i-1)/2);b.writeUInt32LE(i%2?slot:(slot+1)%10,p+84);b.writeBigUInt64LE(BigInt(0x1000+slot*16),p+72);b.writeUInt32LE(123,p+96);}
}
assert.equal(analyze(b).bounded_capture_gate,true);assert.equal(analyze(b).pairs,10);
const bad=Buffer.from(b);bad.writeUInt32LE(999,72+2*280+96);assert.equal(analyze(bad).bounded_capture_gate,false);
const overflow=Buffer.from(b);overflow.writeBigUInt64LE(1n,48);assert.equal(analyze(overflow).bounded_capture_gate,false);
const restore=Buffer.from(b);restore.writeBigUInt64LE(0n,40);assert.equal(analyze(restore).bounded_capture_gate,false);
assert.throws(()=>analyze(b.subarray(0,b.length-1)));
console.log('PASS: ten paired ring slots including wrap; reject changed value, overflow, failed restoration and truncation.');
