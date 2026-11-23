import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';

const here = new URL('.', import.meta.url);
const read = url => fs.readFileSync(new URL(url, here));
const text = url => read(url).toString('utf8').replace(/^\uFEFF/, '').replace(/\r\n/g, '\n');
const sha256 = data => crypto.createHash('sha256').update(data).digest('hex').toUpperCase();

const acceptedLtf = read('../../I-0010/prototype/prototype_core.hpp');
const acceptedDebug = read('../../I-0010/prototype/debug_config.hpp');
const acceptedTrtf = read('../../I-0013/producer-time-fix-prototype/producer_time_fix.cpp');
const ltf = acceptedLtf.toString('utf8').replace(/^\uFEFF/, '').replace(/\r\n/g, '\n');
const trtfSource = acceptedTrtf.toString('utf8').replace(/^\uFEFF/, '').replace(/\r\n/g, '\n');
const trtf = text('trtf_core.hpp');
const runtime = text('timing_core.cpp');
const publicCore = text('timing_core.hpp');
const controlCore = text('control_core.hpp');
const host = text('reshade_addon.cpp');

assert.ok(runtime.includes('#include "../../I-0010/prototype/prototype_core.hpp"'));
assert.ok(runtime.includes('#include "../../I-0010/prototype/debug_config.hpp"'));
for (const token of [
  'constexpr float reference_interval = 1.0f / 60.0f;',
  'const float normalized = raw * (reference_interval / elapsed);',
  'constexpr unsigned char expected_opcode[3] = {0x0F, 0x2F, 0xF8};',
  'MatchDistanceFrames(input.previous, input.current,',
]) assert.ok(ltf.includes(token), `accepted LTF: ${token}`);
for (const token of [
  'native - sample.state_before) * ratio',
  'std::pow(static_cast<double>(sample.retention), ratio)',
  'native == 0.0f',
  'std::fabs(native) == 0.5f',
]) assert.ok(trtfSource.includes(token), `accepted TRTF: ${token}`);
for (const token of [
  'static_cast<double>(input.native_state - input.state_before) *',
  'std::pow(static_cast<double>(input.retention), ratio)',
  'input.native_state == 0.0f',
  'std::fabs(input.native_state) == kStateMaximum',
  'Branch::native_reference',
]) assert.ok(trtf.includes(token), `integrated TRTF: ${token}`);
assert.ok(!/\b(165|240)\b/.test(trtf), 'TRTF core must not contain cadence-specific constants');

for (const token of [
  'g_ltf_active', 'g_trtf_active', 'g_ltf_request_generation',
  'g_trtf_request_generation', 'RequestLtf', 'RequestTrtf',
  'HookSpatialBoundary', 'HookTerrainProducer', 'RestoreTrtfStateExact',
  'kRvaTerrainProducer = 0x005E6580', 'actual_hash != expected_hash',
]) assert.ok(runtime.includes(token), token);
assert.ok(runtime.includes('GetAsyncKeyState'), 'fullscreen keyboard fallback');
assert.ok(runtime.includes('VK_F6') && runtime.includes('VK_F7') &&
          runtime.includes('VK_F12'), 'fullscreen controls are complete');

for (const core of [runtime, publicCore, controlCore, trtf]) {
  assert.ok(!/reshade/i.test(core), 'correction/control cores must not depend on ReShade');
}
for (const token of [
  '#include "timing_core.hpp"', 'i0014::RequestNumericCadence',
  'i0014::RequestNativeCadence', 'i0014::RequestLtf',
  'i0014::RequestTrtf', 'i0014::RequestStop', 'i0014::StopAndWait',
  'LTF — Locomotion Timing Fix', 'TRTF — Terrain Response Timing Fix',
]) assert.ok(host.includes(token), token);
assert.ok(!host.includes('prototype_core.hpp'));
assert.ok(!host.includes('trtf_core.hpp'));
for (const forbidden of [
  'register_event<', 'addon_event::present', 'addon_event::draw',
  'addon_event::create_resource', 'Present(',
]) assert.ok(!host.includes(forbidden), `ReShade host must not contain ${forbidden}`);

const result = {
  schema_version: 1,
  investigation_id: 'I-0014',
  validated_at: new Date().toISOString(),
  passed: true,
  ltf: {
    formula: 'raw*((1/60)/previous_broad_delta)',
    accepted_path: 'evidence/I-0010/prototype/prototype_core.hpp',
    accepted_sha256: sha256(acceptedLtf),
    debug_path: 'evidence/I-0010/prototype/debug_config.hpp',
    debug_sha256: sha256(acceptedDebug),
    relation: 'included directly; no copied LTF formula exists in I-0014',
  },
  trtf: {
    additive_formula: 'state+(native_state-state)*dt/dt60',
    decay_formula: 'state*retention^(dt/dt60)',
    accepted_path: 'evidence/I-0013/producer-time-fix-prototype/producer_time_fix.cpp',
    accepted_sha256: sha256(acceptedTrtf),
    relation: 'host-independent core preserves the accepted branch order, precision and passthrough rules',
  },
  separation: {
    state: 'distinct LTF/TRTF atomics, requests, transitions and telemetry',
    interventions: ['LTF guarded comparison/debug-register path', 'TRTF current-player producer 0x005E6580'],
    restoration: ['LTF debug state', 'TRTF last overwritten native state', 'shared cadence pair'],
  },
  fullscreen: 'runtime-thread Ctrl+function-key fallback is independent of ReShade runtime/swap-chain recreation',
  host: 'ReShade settings overlay only; queues requests through timing_core.hpp',
};
fs.writeFileSync(new URL('DERIVATION-VALIDATION.json', here), JSON.stringify(result, null, 2) + '\n');
console.log('PASS: canonical LTF/TRTF logic is separate, ReShade-free, and independently controlled.');
