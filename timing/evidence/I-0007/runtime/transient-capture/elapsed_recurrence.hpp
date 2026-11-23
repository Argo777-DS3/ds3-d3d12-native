#pragma once
#include <cmath>

// Experimental continuous interpolation of the native60 recurrence.
// Not a whole-engine fixed timestep. Native endpoints preserve float32 resets.
struct ElapsedRecurrence {
    float value = 1.0f, start = 1.0f, end = 1.0f;
    double phase = 0.0;
    bool initialized = false, has_branch = false, reduction = false;
    static constexpr double reference = 1.0 / 60.0;
    static constexpr double tolerance = reference * 1e-5;

    static float Native(float s, bool active, bool reduce) {
        if (!active) return s;
        return reduce ? (s > 0.5f ? s * 0.8f : 0.5f)
                      : (s < 1.0f ? s * 1.2f : 1.0f);
    }
    bool Seed(float s) {
        if (!std::isfinite(s) || s < 0.0f || s > 1.21f) return false;
        value = start = end = s; phase = 0; has_branch = false;
        initialized = true; return true;
    }
    bool Advance(bool active, bool reduce, double dt) {
        if (!initialized || !std::isfinite(dt) || dt <= 0 || dt > 0.05) return false;
        if (!active) return true;
        if (!has_branch || reduce != reduction) {
            start = value; end = Native(start, true, reduce); phase = 0;
            reduction = reduce; has_branch = true;
        }
        phase += dt;
        while (phase + tolerance >= reference) {
            value = end; start = end;
            phase = phase > reference ? phase - reference : 0;
            end = Native(start, true, reduce);
        }
        if (phase > 0) value = static_cast<float>(
            static_cast<double>(start) + (static_cast<double>(end) - start) * phase / reference);
        return std::isfinite(value) && value >= 0 && value <= 1.21f;
    }
};
