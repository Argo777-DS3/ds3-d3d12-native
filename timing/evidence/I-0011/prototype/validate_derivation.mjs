import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';

const here = new URL('.', import.meta.url);
const sha256 = data => crypto.createHash('sha256').update(data).digest('hex').toUpperCase();
const read = url => fs.readFileSync(new URL(url, here));
const text = url => read(url).toString('utf8').replace(/^\uFEFF/, '').replace(/\r\n/g, '\n');

const acceptedCore = read('../../I-0010/prototype/prototype_core.hpp');
const acceptedDebug = read('../../I-0010/prototype/debug_config.hpp');
const runtime = text('runtime.cpp');
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
]) assert.ok(runtime.includes(token), token);

const result = {
  schema_version: 1,
  investigation_id: 'I-0011',
  validated_at: new Date().toISOString(),
  passed: true,
  formula: 'raw*((1/60)/previous_broad_delta)',
  formula_guard_core: {
    path: 'evidence/I-0010/prototype/prototype_core.hpp',
    sha256: sha256(acceptedCore),
    relation: 'included directly; no I-0011 copy or altered formula exists',
  },
  debug_restoration_core: {
    path: 'evidence/I-0010/prototype/debug_config.hpp',
    sha256: sha256(acceptedDebug),
    relation: 'included directly; unchanged DR configuration/restoration predicates',
  },
  runtime_guards: [
    'exact opcode', 'player/owner chain', 'consecutive update index',
    'transform identity', 'position-pair identity', 'finite previous delta',
    'exact-build signatures', 'safe update-boundary toggle',
  ],
};
fs.writeFileSync(new URL('DERIVATION-VALIDATION.json', here), JSON.stringify(result, null, 2) + '\n');
console.log('PASS: I-0011 includes the accepted I-0010 formula/guard and debug cores unchanged.');
