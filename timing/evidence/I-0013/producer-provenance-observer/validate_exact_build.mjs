import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
const game='C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/DarkSoulsIII.exe';
const binary=fs.readFileSync(game);
const sha256=crypto.createHash('sha256').update(binary).digest('hex').toUpperCase();
assert.equal(sha256,'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C');
const pe=binary.readUInt32LE(0x3c), sections=pe+24+binary.readUInt16LE(pe+20), count=binary.readUInt16LE(pe+6);
function offset(rva,n){for(let i=0;i<count;i++){const s=sections+i*40,b=binary.readUInt32LE(s+12),z=binary.readUInt32LE(s+16);if(rva>=b&&rva+n<=b+z)return binary.readUInt32LE(s+20)+rva-b;}throw new Error('not file backed');}
const expected=Buffer.from('48895C24084889742410574881EC90000000','hex');
const at=offset(0x005C8100,expected.length); assert.deepEqual(binary.subarray(at,at+expected.length),expected);
process.stdout.write(`${JSON.stringify({schema_version:1,investigation_id:'I-0013',status:'pass',game_sha256:sha256,hook_rva:'0x005C8100',bytes:expected.toString('hex').toUpperCase(),limit:'Exact-build static validation only.'},null,2)}\n`);
