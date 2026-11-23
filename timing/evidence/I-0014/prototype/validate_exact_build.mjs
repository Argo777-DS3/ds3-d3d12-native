import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';

const here = new URL('.', import.meta.url);
const gameCandidates = [
  'C:/Program Files (x86)/Steam/steamapps/common/DARK SOULS III/Game/DarkSoulsIII.exe',
  new URL('../../I-0002/artifacts/staging/DarkSoulsIII.exe', here),
];
const game = gameCandidates.find(candidate => fs.existsSync(candidate));
assert.ok(game, 'exact-build executable is unavailable');
const binary = fs.readFileSync(game);
const sha256 = data => crypto.createHash('sha256').update(data).digest('hex').toUpperCase();
const expectedHash = 'EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C';
assert.equal(sha256(binary), expectedHash);

const pe = binary.readUInt32LE(0x3c);
assert.equal(binary.readUInt32LE(pe), 0x4550);
const sectionCount = binary.readUInt16LE(pe + 6);
const sections = pe + 24 + binary.readUInt16LE(pe + 20);
const fileOffset = (rva, length) => {
  for (let i = 0; i < sectionCount; i++) {
    const section = sections + i * 40;
    const base = binary.readUInt32LE(section + 12);
    const size = binary.readUInt32LE(section + 16);
    if (rva >= base && rva + length <= base + size) {
      return binary.readUInt32LE(section + 20) + rva - base;
    }
  }
  throw new Error(`RVA 0x${rva.toString(16)} is not file-backed`);
};
const signatures = [
  { name: 'broad_update_hook', rva: 0x00D57900, bytes: '488BC4574883EC5048C740C8FEFFFFFF' },
  { name: 'spatial_boundary_hook', rva: 0x02344730, bytes: '405556574154415541564157488BEC' },
  { name: 'causal_comparison', rva: 0x005C73D8, bytes: '0F2FF8762FF30F10835C0200000F2F' },
  { name: 'recurrence_exit', rva: 0x005C7435, bytes: '488B4C24404833CCE8DEA2A0014C8D' },
  { name: 'terrain_response_producer', rva: 0x005E6580, bytes: '40555356488D6C24C14881ECC0000000' },
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

const runtime = fs.readFileSync(new URL('timing_core.cpp', here), 'utf8');
for (const token of [
  'kRvaCentralGlobal = 0x0489DD10',
  'kRvaComparison = 0x005C73D8',
  'kRvaRecurrenceExit = 0x005C7435',
  'kRvaTerrainProducer = 0x005E6580',
  'RuntimeState::standby',
  'unlimited_unsupported',
  'RestoreTimingExact()',
  'RestoreTrtfStateExact()',
  'actual_hash != expected_hash',
]) assert.ok(runtime.includes(token), token);

const build = JSON.parse(fs.readFileSync(new URL('BUILD-VALIDATION.json', here), 'utf8').replace(/^\uFEFF/, ''));
assert.equal(build.start_state, 'STANDBY');
assert.equal(build.start_inert, true);
assert.equal(build.start_ltf, false);
assert.equal(build.start_trtf, false);
assert.equal(build.native_test.passed, true);
assert.equal(build.derivation.passed, true);
for (const artifact of build.binaries) {
  const data = fs.readFileSync(new URL(artifact.path, here));
  assert.equal(sha256(data), artifact.sha256, artifact.path);
}

const result = {
  schema_version: 1,
  investigation_id: 'I-0014',
  validated_at: new Date().toISOString(),
  game_path: typeof game === 'string' ? game : game.pathname,
  game_sha256: expectedHash,
  runtime_full_hash_guard: true,
  signatures,
  start_state: 'STANDBY',
  start_inert: true,
  numeric_range: [30, 360],
  presets: [30, 60, 90, 120, 144, 165, 240],
  unlimited: 'unsupported: no proven limiter-off representation; no fake cap',
  correction_hooks: {
    ltf: ['0x00D57900', '0x02344730'],
    trtf: ['0x005E6580'],
  },
  host: 'ReShade 6.8.0 API 20 settings overlay plus renderer-independent keyboard fallback',
  display_modes: ['windowed', 'exclusive fullscreen structural support; manual confirmation pending'],
  live_validation: 'not performed; Thomas will run the first test independently',
};
fs.writeFileSync(new URL('EXACT-BUILD-VALIDATION.json', here), JSON.stringify(result, null, 2) + '\n');
console.log(JSON.stringify(result, null, 2));
