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
const root=process.argv[4];let files=0,instructions=0;const failures=[];
function walk(dir){for(const entry of fs.readdirSync(dir,{withFileTypes:true})){const file=dir+'/'+entry.name;if(entry.isDirectory())walk(file);else if(file.endsWith('.asm.txt')){
 let hits=0;for(const line of fs.readFileSync(file,'utf8').split(/\r?\n/)){const match=line.match(/^([0-9A-F]+)  ([0-9a-f]+)  /);if(!match)continue;const rva=parseInt(match[2],16)-imageBase;const bytes=Buffer.from(match[1],'hex');const offset=offsetForRva(rva);if(offset===null||!image.subarray(offset,offset+bytes.length).equals(bytes))failures.push({file,rva:hex(rva)});hits++;}if(hits){files++;instructions+=hits;}
}}}walk(root);
const result={image_sha256:crypto.createHash('sha256').update(image).digest('hex').toUpperCase(),files,instructions,failures,limit:'Validates exported instruction bytes against the canonical PE, not inferred function boundaries or semantics.'};fs.writeFileSync(outputFile,JSON.stringify(result,null,2)+'\n');console.log(JSON.stringify(result));if(failures.length)process.exitCode=1;
