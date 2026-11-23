import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';

const game = 'C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/DarkSoulsIII.exe';
const binary = fs.readFileSync(game);
const sha256 = crypto.createHash('sha256').update(binary).digest('hex').toUpperCase();
assert.equal(sha256, 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C');
const pe = binary.readUInt32LE(0x3c);
const sections = pe + 24 + binary.readUInt16LE(pe + 20);
const count = binary.readUInt16LE(pe + 6);
function offset(rva, size) {
  for (let i = 0; i < count; i++) {
    const section = sections + i * 40;
    const base = binary.readUInt32LE(section + 12);
    const rawSize = binary.readUInt32LE(section + 16);
    if (rva >= base && rva + size <= base + rawSize) {
      return binary.readUInt32LE(section + 20) + rva - base;
    }
  }
  throw new Error('RVA is not file-backed');
}
const expected = Buffer.from('48895C24084889742410574881EC90000000', 'hex');
const at = offset(0x005C8100, expected.length);
assert.deepEqual(binary.subarray(at, at + expected.length), expected);
process.stdout.write(`${JSON.stringify({
  schema_version: 1,
  investigation_id: 'I-0013',
  status: 'pass',
  game_sha256: sha256,
  hook_rva: '0x005C8100',
  bytes: expected.toString('hex').toUpperCase(),
  limit: 'Exact-build static validation only.',
}, null, 2)}\n`);
