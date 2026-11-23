import assert from 'node:assert/strict';
import {
  compareRoute, parseCaptureCsv, resampleByProgress, summarizeCapture,
} from './analyze_capture.mjs';

const frequency = 1_000_000;

function makeTrace(fps, zAt, invalidAt = new Set()) {
  const samples = [];
  for (let index = 0; index <= fps * 2; index++) {
    const u = index / (fps * 2);
    samples.push({
      qpc: 1_000_000 + Math.round(index * frequency / fps),
      updateIndex: 1000 + index,
      player: '0x1000', transform: '0x2000', delta: 1 / fps,
      x: u * 10, y: 0, z: zAt(u, index),
      valid: !invalidAt.has(index), targetFps: fps,
      correction: false,
    });
  }
  return samples;
}

const smooth60 = summarizeCapture(makeTrace(60, u => 0.5 * u), frequency);
const smooth240 = summarizeCapture(makeTrace(240, u => 0.5 * u), frequency);
assert.ok(smooth60.vertical_excess < 1e-12);
assert.ok(smooth240.vertical_reversal_count === 0);
assert.ok(compareRoute(smooth60, smooth240).route_shape_rms < 1e-9);

const stairs60 = summarizeCapture(
  makeTrace(60, u => Math.floor(u * 10) * 0.05), frequency);
const stairs240 = summarizeCapture(
  makeTrace(240, u => Math.floor(u * 10) * 0.05), frequency);
assert.equal(stairs60.vertical_reversal_count, 0);
assert.ok(compareRoute(stairs60, stairs240).route_shape_rms < 0.01);

const jitter240 = summarizeCapture(
  makeTrace(240, (u, index) => 0.5 * u + (index % 2 ? 0.003 : -0.003)),
  frequency);
assert.ok(jitter240.vertical_excess > 1.0);
assert.ok(jitter240.vertical_reversal_count > 400);
assert.ok(compareRoute(smooth60, jitter240).route_shape_detrended_rms > 0.002);

const invalid = summarizeCapture(
  makeTrace(60, u => 0.5 * u, new Set([20, 21, 22])), frequency);
assert.equal(invalid.invalid_rows, 3);
assert.equal(invalid.valid_rows, 118);

assert.throws(() => resampleByProgress(makeTrace(60, () => 0)
  .map(sample => ({ ...sample, x: 0, y: 0 }))));

const csv = [
  'qpc,update_index,player,transform,delta,x,z,y,valid,target_fps,correction',
  '100,1,0x1,0x2,0.016,1,2,3,1,60,0',
  '200,2,0x1,0x2,0.016,2,2.1,3,1,60,0',
].join('\n');
assert.equal(parseCaptureCsv(csv).length, 2);

console.log(JSON.stringify({
  passed: true,
  cases: ['smooth-grade-cadence-invariance', 'monotonic-stairs',
    'alternating-jitter-detection', 'invalid-row-exclusion',
    'zero-route-rejection', 'csv-schema'],
  detection: {
    smooth_shape_rms: compareRoute(smooth60, smooth240).route_shape_rms,
    stair_shape_rms: compareRoute(stairs60, stairs240).route_shape_rms,
    jitter_detrended_rms:
      compareRoute(smooth60, jitter240).route_shape_detrended_rms,
    jitter_vertical_excess: jitter240.vertical_excess,
  },
}, null, 2));
