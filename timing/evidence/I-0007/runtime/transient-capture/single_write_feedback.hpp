#pragma once
#include "elapsed_recurrence.hpp"
#include "../causal-patch/distance_normalization.hpp"
#include <limits>

// Isolated proposal. No memory hooks or game writes. The adapter must still
// qualify caller, observed branch coverage and distance-frame correspondence.
class SingleWriteFeedback {
    ElapsedRecurrence state_;
    uintptr_t owner_ = 0;
    uint64_t epoch_ = 0, next_ = 0;
    bool ready_ = false;
public:
    bool Reset(uintptr_t owner, uint64_t epoch, uint64_t first, float seed) {
        ElapsedRecurrence next;
        if (!owner || !epoch || !first || !next.Seed(seed)) return false;
        state_ = next; owner_ = owner; epoch_ = epoch; next_ = first; ready_ = true;
        return true;
    }
    float Value() const { return state_.value; }
    bool Advance(uintptr_t owner, uint64_t epoch, uint64_t index, float before,
                 bool active, bool compared, float raw, float distance_dt,
                 double recurrence_dt, float native_exit, float* output) {
        if (!ready_ || !output || owner != owner_ || epoch != epoch_ || index != next_ ||
            index == std::numeric_limits<uint64_t>::max() || before != state_.value ||
            (compared && !active)) return false;
        float corrected = 0;
        if (compared && !NormalizeDistanceTest(raw, distance_dt, &corrected)) return false;
        const bool raw_reduce = compared && raw < 1.0f;
        const bool desired_reduce = compared && corrected < 1.0f;
        if (native_exit != ElapsedRecurrence::Native(before, active, raw_reduce)) return false;
        ElapsedRecurrence next = state_;
        if (!next.Advance(active, desired_reduce, recurrence_dt)) return false;
        // All fallible validation precedes local state/output commit. Rejection
        // leaves both untouched; the future adapter must abort its live trial.
        state_ = next; ++next_; *output = next.value;
        return true;
    }
};
