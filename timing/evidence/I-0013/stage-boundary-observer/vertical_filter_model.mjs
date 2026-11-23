const referenceDt = 1 / 60;
const baseWindow = 0.03;
const smoothingGain = 0.2;
const decayGain = 0.6;

export const constants = Object.freeze({
  referenceDt,
  baseWindow,
  smoothingGain,
  decayGain,
  referenceAlpha: smoothingGain * (referenceDt / baseWindow),
  referenceDecay: decayGain * (referenceDt / baseWindow),
});

export function nativeAlpha(dt) {
  return smoothingGain * (dt / baseWindow);
}

export function nativeDecay(dt) {
  return decayGain * (dt / baseWindow);
}

export function invariantAlpha(dt) {
  return 1 - Math.pow(1 - constants.referenceAlpha, dt / referenceDt);
}

export function invariantDecay(dt) {
  return Math.pow(constants.referenceDecay, dt / referenceDt);
}

export function stepOutput(previous, target, dt, invariant = false) {
  const alpha = invariant ? invariantAlpha(dt) : nativeAlpha(dt);
  return previous + (target - previous) * alpha;
}

export function stepGate(previous, dt, invariant = false) {
  const retention = invariant ? invariantDecay(dt) : nativeDecay(dt);
  return previous * retention;
}

export function compareRates(rates = [30, 60, 120, 240, 360]) {
  return rates.map((rate) => {
    const dt = 1 / rate;
    const alphaNative = nativeAlpha(dt);
    const alphaInvariant = invariantAlpha(dt);
    const decayNative = nativeDecay(dt);
    const decayInvariant = invariantDecay(dt);
    return {
      rate,
      dt,
      output_alpha_native: alphaNative,
      output_alpha_invariant: alphaInvariant,
      output_retention_after_one_second_native: Math.pow(1 - alphaNative, rate),
      output_retention_after_one_second_invariant: Math.pow(1 - alphaInvariant, rate),
      gate_decay_per_update_native: decayNative,
      gate_decay_per_update_invariant: decayInvariant,
      gate_retention_after_one_second_native: Math.pow(decayNative, rate),
      gate_retention_after_one_second_invariant: Math.pow(decayInvariant, rate),
    };
  });
}

if (process.argv[1]?.endsWith('vertical_filter_model.mjs')) {
  process.stdout.write(`${JSON.stringify({
    schema_version: 1,
    constants,
    rates: compareRates(),
    candidate: {
      output: 'alpha(dt) = 1 - (8/9)^(60*dt)',
      gate: 'retention(dt) = (1/3)^(60*dt)',
    },
    limit: 'Static recurrence comparison only; runtime activation and causality require observation.'
  }, null, 2)}\n`);
}
