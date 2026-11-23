import assert from 'node:assert/strict';
import crypto from 'node:crypto';
import fs from 'node:fs';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const baseCommit = 'e81316e28bede7a431ad39713b77f71ab3d046ed';
const here = path.dirname(fileURLToPath(import.meta.url));
const repoRoot = path.resolve(here, '../../..');
const sha256 = data => crypto.createHash('sha256').update(data).digest('hex').toUpperCase();
const normalize = text => text.replace(/^\uFEFF/, '').replace(/\r\n/g, '\n');

const baseText = name => normalize(execFileSync(
  'git', ['show', `${baseCommit}:evidence/I-0009/prototype/${name}`],
  { cwd: repoRoot, encoding: 'utf8' },
));
const derivedText = name => normalize(fs.readFileSync(path.join(here, name), 'utf8'));
const checkedOutBaseBytes = name => fs.readFileSync(
  path.resolve(here, '../../I-0009/prototype', name),
);

const records = [];
const requireIdentical = name => {
  const base = baseText(name);
  const derivedBytes = fs.readFileSync(path.join(here, name));
  const checkedOutBytes = checkedOutBaseBytes(name);
  assert.deepEqual(derivedBytes, checkedOutBytes,
                   `${name} must be byte-identical to checked-out I-0009`);
  const derived = normalize(derivedBytes.toString('utf8'));
  assert.equal(derived, base, `${name} must remain byte-content equivalent to I-0009`);
  records.push({
    path: name,
    relation: 'byte_identical_to_checked_out_i0009_and_content_identical_to_e81316e',
    base_commit_normalized_sha256: sha256(base),
    checked_out_i0009_sha256: sha256(checkedOutBytes),
    derived_sha256: sha256(derivedBytes),
  });
};
const requireTransform = (name, transform, changes) => {
  const base = baseText(name);
  const expected = transform(base);
  const derived = derivedText(name);
  assert.equal(derived, expected, `${name} contains a non-allowlisted I-0009 difference`);
  records.push({
    path: name,
    relation: 'exact_allowlisted_transform',
    allowlisted_changes: changes,
    base_sha256: sha256(base),
    derived_sha256: sha256(derived),
  });
};

requireIdentical('prototype_core.hpp');
requireIdentical('debug_config.hpp');

requireTransform('prototype.cpp', text => text
  .replace('constexpr float kTargetFps = 165.0f;', 'constexpr float kTargetFps = 240.0f;')
  .replaceAll('DS3_I0009', 'DS3_I0010')
  .replaceAll('target_fps=165', 'target_fps=240')
  .replace('schema=I-0009-visible-locomotion-prototype-v1',
           'schema=I-0010-locomotion-240-validation-v1'), [
    'target constant 165 -> 240',
    'named-object namespace I0009 -> I0010',
    'disk/session target identity 165 -> 240',
    'session schema identity I-0009 -> I-0010',
  ]);

requireTransform('injector.cpp', text => text
  .replaceAll('DS3_I0009', 'DS3_I0010')
  .replaceAll('I-0009', 'I-0010')
  .replaceAll('I0009PrototypeController', 'I0010PrototypeController'), [
    'named-object namespace I0009 -> I0010',
    'controller/user-facing identity I-0009 -> I-0010',
  ]);

requireTransform('public_status.hpp', text => text
  .replace('0x49303039u;  // I009', '0x49303130u;  // I010')
  .replace('requested_fps = 165.0f;', 'requested_fps = 240.0f;'), [
    'public status magic I009 -> I010',
    'reported target 165 -> 240',
  ]);

requireTransform('prototype_self_test.cpp', text => text
  .replaceAll('1.0f / 165.0f', '1.0f / 240.0f')
  .replace('decision.normalized - 1.375f', 'decision.normalized - 2.0f'), [
    'test elapsed target 165 -> 240',
    'mathematically corresponding expected normalized value 1.375 -> 2.0',
  ]);

requireTransform('prototype-config.txt', text =>
  text.replace('target_fps=165', 'target_fps=240'), [
  'disabled configuration target 165 -> 240',
]);

const core = derivedText('prototype_core.hpp');
for (const token of [
  'constexpr float reference_interval = 1.0f / 60.0f;',
  'const float normalized = raw * (reference_interval / elapsed);',
  'constexpr unsigned char expected_opcode[3] = {0x0F, 0x2F, 0xF8};',
  'std::memcmp(input.owner_position, input.getter_position, 12) != 0',
  'MatchDistanceFrames(input.previous, input.current,',
]) assert.ok(core.includes(token), token);

const result = {
  schema_version: 1,
  investigation_id: 'I-0010',
  validated_at: new Date().toISOString(),
  base_commit: baseCommit,
  passed: true,
  immutable_contract: {
    formula: 'raw*((1/60)/previous_broad_delta)',
    comparison_rva: '0x005C73D8',
    recurrence_exit_rva: '0x005C7435',
    formula_and_guard_core: 'prototype_core.hpp identical to I-0009',
    debug_restoration_core: 'debug_config.hpp identical to I-0009',
  },
  records,
};
fs.writeFileSync(path.join(here, 'DERIVATION-VALIDATION.json'),
                 JSON.stringify(result, null, 2) + '\n');
console.log('PASS: I-0010 runtime is the exact allowlisted target/identity derivative of I-0009.');
