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

const source = fs.readFileSync(new URL('prototype.cpp', here), 'utf8');
for (const token of [
  'kTargetFps = 240.0f',
  'kRvaComparison = 0x005C73D8',
  'kRvaRecurrenceExit = 0x005C7435',
  'kRvaCentralGlobal = 0x0489DD10',
  'SafeRead(central + 0x268, &central_delta)',
  'ReadPointer(modules + 0x10, &animation)',
  'SafeRead(animation + 0x24, &snapshot.animation_clock)',
  'PrototypeState::standby',
  'PrototypeState::correct',
  'comparison_guard_failed',
]) assert.ok(source.includes(token), token);

const config = fs.readFileSync(new URL('prototype-config.txt', here), 'utf8');
assert.match(config, /^target_fps=240$/m);
assert.match(config, /^start_enabled=0$/m);

const build = JSON.parse(fs.readFileSync(new URL('BUILD-VALIDATION.json', here), 'utf8').replace(/^\uFEFF/, ''));
assert.equal(build.start_state, 'STANDBY');
assert.equal(build.start_enabled, false);
assert.equal(build.native_test.passed, true);
assert.equal(build.i0009_derivation.passed, true);
assert.equal(build.i0009_derivation.base_commit,
             'e81316e28bede7a431ad39713b77f71ab3d046ed');
for (const artifact of build.binaries) {
  const data = fs.readFileSync(new URL(artifact.path, here));
  assert.equal(sha256(data), artifact.sha256, artifact.path);
}

const result = {
  schema_version: 1,
  investigation_id: 'I-0010',
  validated_at: new Date().toISOString(),
  game_path: typeof game === 'string' ? game : game.pathname,
  game_sha256: expectedHash,
  signatures,
  prototype_start_state: 'STANDBY',
  correction_start_enabled: false,
  controls: ['prepare', 'enable', 'disable', 'status', 'stop'],
  i0008_reuse: {
    central_delta: 'central object +0x268 must match the I-0007 broad delta within 1e-7',
    animation_sentinel: 'player modules+0x10 -> +0x24 valid/change counters; observation only',
  },
  native_test: build.native_test,
  live_validation: 'not yet performed',
};
fs.writeFileSync(new URL('EXACT-BUILD-VALIDATION.json', here), JSON.stringify(result, null, 2) + '\n');
console.log(JSON.stringify(result, null, 2));
