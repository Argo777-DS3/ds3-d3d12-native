import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';

const here = new URL('.', import.meta.url);
const sha256 = data => crypto.createHash('sha256').update(data).digest('hex').toUpperCase();
const read = url => fs.readFileSync(new URL(url, here));
const text = url => read(url).toString('utf8').replace(/^\uFEFF/, '').replace(/\r\n/g, '\n');

const acceptedCore = read('../../I-0010/prototype/prototype_core.hpp');
const acceptedDebug = read('../../I-0010/prototype/debug_config.hpp');
const runtime = text('timing_core.cpp');
const host = text('reshade_addon.cpp');
const publicCore = text('timing_core.hpp');
const controlCore = text('control_core.hpp');
const core = acceptedCore.toString('utf8').replace(/^\uFEFF/, '').replace(/\r\n/g, '\n');

assert.ok(runtime.includes('#include "../../I-0010/prototype/prototype_core.hpp"'));
assert.ok(runtime.includes('#include "../../I-0010/prototype/debug_config.hpp"'));
for (const token of [
  'constexpr float reference_interval = 1.0f / 60.0f;',
  'const float normalized = raw * (reference_interval / elapsed);',
  'constexpr unsigned char expected_opcode[3] = {0x0F, 0x2F, 0xF8};',
  'std::memcmp(input.owner_position, input.getter_position, 12) != 0',
  'MatchDistanceFrames(input.previous, input.current,',
]) assert.ok(core.includes(token), token);
for (const token of [
  'input.previous = g_distance_previous;',
  'input.current = g_distance_current;',
  'input.expected_index = g_watch_update_index;',
  'input.expected_transform = g_watch_transform_base - 0xE0;',
  'EvaluateComparison(input, &decision)',
  'context->Xmm0.Low =',
  'correction_transition_while_armed',
  'actual_hash != expected_hash',
]) assert.ok(runtime.includes(token), token);

assert.ok(!/reshade/i.test(runtime), 'timing core must not depend on ReShade');
assert.ok(!/reshade/i.test(publicCore), 'public core API must not depend on ReShade');
assert.ok(!/reshade/i.test(controlCore), 'control model must not depend on ReShade');
for (const token of [
  '#include "timing_core.hpp"',
  'i0012::RequestNumericCadence',
  'i0012::RequestNativeCadence',
  'i0012::RequestCorrection',
  'i0012::RequestStop',
  'i0012::StopAndWait',
]) assert.ok(host.includes(token), token);
assert.ok(!host.includes('prototype_core.hpp'));
assert.ok(!host.includes('debug_config.hpp'));
for (const forbidden of [
  'register_event<', 'addon_event::present', 'addon_event::draw',
  'addon_event::create_resource', 'Present(',
]) assert.ok(!host.includes(forbidden), `ReShade host must not contain ${forbidden}`);

const result = {
  schema_version: 1,
  investigation_id: 'I-0012',
  validated_at: new Date().toISOString(),
  passed: true,
  formula: 'raw*((1/60)/previous_broad_delta)',
  formula_guard_core: {
    path: 'evidence/I-0010/prototype/prototype_core.hpp',
    sha256: sha256(acceptedCore),
    relation: 'included directly by the host-independent core; no I-0012 formula copy exists',
  },
  debug_restoration_core: {
    path: 'evidence/I-0010/prototype/debug_config.hpp',
    sha256: sha256(acceptedDebug),
    relation: 'included directly by the host-independent core; predicates unchanged',
  },
  architecture: {
    core: 'no ReShade headers, symbols or UI calls',
    host: 'ReShade overlay only; queues requests through timing_core.hpp',
  },
  runtime_guards: [
    'full executable SHA-256', 'four exact native signatures', 'exact opcode',
    'player/owner chain', 'consecutive update index', 'transform identity',
    'position-pair identity', 'finite previous delta',
    'safe update-boundary toggle', 'exact timing/debug restoration',
  ],
};
fs.writeFileSync(new URL('DERIVATION-VALIDATION.json', here), JSON.stringify(result, null, 2) + '\n');
console.log('PASS: ReShade host is isolated from the unchanged accepted formula/guard core.');
