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
function allOccurrences(needle) {
  const found = [];
  let offset = 0;
  while ((offset = image.indexOf(needle, offset)) >= 0) {
    found.push(offset);
    offset += 1;
  }
  return found;
}

function ripRelativeLeaReferences(targetRva) {
  const references = [];
  for (const section of sections.filter((candidate) => (candidate.characteristics & 0x20000000) !== 0)) {
    const end = section.rawOffset + section.rawSize - 7;
    for (let offset = section.rawOffset; offset <= end; offset += 1) {
      if ((image[offset] !== 0x48 && image[offset] !== 0x4c) || image[offset + 1] !== 0x8d || (image[offset + 2] & 0xc7) !== 0x05) continue;
      const instructionRva = section.rva + offset - section.rawOffset;
      const displacement = image.readInt32LE(offset + 3);
      if (instructionRva + 7 + displacement === targetRva) references.push(hex(instructionRva));
    }
  }
  return references;
}

const ascii = image.toString("latin1");
const typeRegex = /\.\?AV[^\0\r\n]{0,180}(?:Anim|Behavior)[^\0\r\n]{0,180}@@/gi;
const types = [];
const seen = new Set();
for (const match of ascii.matchAll(typeRegex)) {
  const typeName = match[0];
  const stringOffset = match.index;
  const typeOffset = stringOffset - 16;
  const typeRva = rvaForOffset(typeOffset);
  if (typeRva === null || seen.has(`${typeRva}:${typeName}`)) continue;
  seen.add(`${typeRva}:${typeName}`);
  const typeNeedle = Buffer.alloc(4);
  typeNeedle.writeUInt32LE(typeRva);
  const vtables = [];
  for (const occurrence of allOccurrences(typeNeedle)) {
    const locatorOffset = occurrence - 12;
    if (locatorOffset < 0 || locatorOffset + 24 > image.length) continue;
    const locatorRva = rvaForOffset(locatorOffset);
    if (locatorRva === null || image.readUInt32LE(locatorOffset) !== 1 || image.readUInt32LE(locatorOffset + 20) !== locatorRva) continue;
    const locatorNeedle = Buffer.alloc(8);
    locatorNeedle.writeBigUInt64LE(BigInt(imageBase + locatorRva));
    for (const pointerOffset of allOccurrences(locatorNeedle)) {
      const pointerRva = rvaForOffset(pointerOffset);
      if (pointerRva === null) continue;
      const vtableRva = pointerRva + 8;
      const vtableOffset = offsetForRva(vtableRva);
      if (vtableOffset === null) continue;
      const entries = [];
      for (let slot = 0; slot < 256 && vtableOffset + slot * 8 + 8 <= image.length; slot += 1) {
        const targetRva = Number(image.readBigUInt64LE(vtableOffset + slot * 8)) - imageBase;
        const targetSection = sectionForRva(targetRva);
        if (!targetSection || (targetSection.characteristics & 0x20000000) === 0) break;
        entries.push({ slot, target_rva: hex(targetRva) });
      }
      if (entries.length) vtables.push({
        locator_rva: hex(locatorRva),
        vtable_rva: hex(vtableRva),
        entry_count: entries.length,
        rip_relative_lea_references: ripRelativeLeaReferences(vtableRva),
        entries: entries.slice(0, 64),
      });
    }
  }
  types.push({ type_name: typeName, type_rva: hex(typeRva), vtables });
}

const output = {
  schema_version: 1,
  image_sha256: crypto.createHash("sha256").update(image).digest("hex").toUpperCase(),
  image_base: hex(imageBase),
  filter: "MSVC class RTTI names containing Anim or Behavior",
  types: types.sort((a, b) => a.type_name.localeCompare(b.type_name)),
  limits: "Exact-build on-disk RTTI/vtable inventory. A matching class name or vtable does not bind the runtime modules+0x10 object or the qualified +0x24 field without an object-chain or live identity join.",
};
fs.writeFileSync(outputFile, `${JSON.stringify(output, null, 2)}\n`);
console.log(JSON.stringify({ type_count: output.types.length, vtable_type_count: output.types.filter((type) => type.vtables.length).length }));
