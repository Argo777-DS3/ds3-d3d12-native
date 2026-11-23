import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';

const game = 'C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/DarkSoulsIII.exe';
const binary = fs.readFileSync(game);
const sha256 = crypto.createHash('sha256').update(binary).digest('hex').toUpperCase();
assert.equal(sha256,
  'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C');

const pe = binary.readUInt32LE(0x3c);
assert.equal(binary.readUInt32LE(pe), 0x4550);
const sectionCount = binary.readUInt16LE(pe + 6);
const sections = pe + 24 + binary.readUInt16LE(pe + 20);
function fileOffset(rva, length) {
  for (let index = 0; index < sectionCount; ++index) {
    const section = sections + index * 40;
    const base = binary.readUInt32LE(section + 12);
    const size = binary.readUInt32LE(section + 16);
    if (rva >= base && rva + length <= base + size) {
      return binary.readUInt32LE(section + 20) + rva - base;
    }
  }
  throw new Error(`RVA 0x${rva.toString(16)} is not file-backed`);
}

const signatures = [
  {
    name: 'base_transform_hook',
    rva: 0x009D97B0,
    bytes: '48895C2418574881ECB0000000488B'
  },
  {
    name: 'aggregator_callsite',
    rva: 0x005C8127,
    bytes: 'E884164100'
  },
].map(item => {
  const expected = Buffer.from(item.bytes, 'hex');
  const offset = fileOffset(item.rva, expected.length);
  const actual = binary.subarray(offset, offset + expected.length);
  assert.deepEqual(actual, expected, item.name);
  return {
    name: item.name,
    rva: `0x${item.rva.toString(16).toUpperCase().padStart(8, '0')}`,
    file_offset: `0x${offset.toString(16).toUpperCase()}`,
    bytes: actual.toString('hex').toUpperCase(),
  };
});

process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  investigation_id: 'I-0013',
  status: 'pass',
  game_sha256: sha256,
  signatures,
  limit: 'Exact-build and static-byte validation only; no live behavior is established.'
}, null, 2)}\n`);
