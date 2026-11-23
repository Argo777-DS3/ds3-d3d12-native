import crypto from "node:crypto";
import fs from "node:fs";

const [imageFile, outputFile] = process.argv.slice(2);
if (!imageFile || !outputFile) throw new Error("usage: node inspect_animation_rtti.mjs <DarkSoulsIII.exe> <output.json>");

const image = fs.readFileSync(imageFile);
const pe = image.readUInt32LE(0x3c);
const sectionCount = image.readUInt16LE(pe + 6);
const optionalHeader = pe + 24;
const optionalHeaderSize = image.readUInt16LE(pe + 20);
const imageBase = Number(image.readBigUInt64LE(optionalHeader + 24));
const sections = [];
for (let index = 0; index < sectionCount; index += 1) {
  const offset = optionalHeader + optionalHeaderSize + index * 40;
  sections.push({
    name: image.subarray(offset, offset + 8).toString("ascii").replace(/\0.*$/, ""),
    virtualSize: image.readUInt32LE(offset + 8),
    rva: image.readUInt32LE(offset + 12),
    rawSize: image.readUInt32LE(offset + 16),
    rawOffset: image.readUInt32LE(offset + 20),
    characteristics: image.readUInt32LE(offset + 36),
  });
}

const hex = (value) => `0x${value.toString(16).toUpperCase()}`;
function sectionForRva(rva) {
  return sections.find((section) => rva >= section.rva && rva < section.rva + Math.max(section.virtualSize, section.rawSize));
}
function rvaForOffset(offset) {
  const section = sections.find((candidate) => offset >= candidate.rawOffset && offset < candidate.rawOffset + candidate.rawSize);
  return section ? section.rva + offset - section.rawOffset : null;
}
function offsetForRva(rva) {
  const section = sectionForRva(rva);
  return section ? section.rawOffset + rva - section.rva : null;
}
const targets=new Set(process.argv.slice(4).length ? process.argv.slice(4).map(Number) : [0x9b51e0,0x9b4eb0]);
const refs=[];
for(const section of sections.filter(s=>s.characteristics & 0x20000000)) {
 for(let off=section.rawOffset;off<section.rawOffset+section.rawSize-7;off++) {
  const rva=section.rva+off-section.rawOffset;
  if(image[off]===0xe8 || image[off]===0xe9) {
   const target=rva+5+image.readInt32LE(off+1);
   if(targets.has(target)) refs.push({kind:'relative_call_or_jump_candidate',rva:hex(rva),target:hex(target)});
  }
  if((image[off]===0x48 || image[off]===0x4c) && image[off+1]===0x8d && (image[off+2]&0xc7)===5) {
   const target=rva+7+image.readInt32LE(off+3);
   if(targets.has(target)) refs.push({kind:'rip_lea_candidate',rva:hex(rva),target:hex(target)});
  }
 }
}
for(const target of targets) {
 const needle=Buffer.alloc(8);needle.writeBigUInt64LE(BigInt(imageBase+target));
 let off=-1;while((off=image.indexOf(needle,off+1))>=0) refs.push({kind:'absolute_pointer_candidate',rva:hex(rvaForOffset(off)),target:hex(target)});
}
const result={image_sha256:crypto.createHash('sha256').update(image).digest('hex').toUpperCase(),targets:[...targets].map(hex),refs,limit:'Only explicitly supplied exact targets; raw matches require instruction validation. No displacement scan.'};
fs.writeFileSync(outputFile,JSON.stringify(result,null,2)+'\n'); console.log(JSON.stringify(result));

