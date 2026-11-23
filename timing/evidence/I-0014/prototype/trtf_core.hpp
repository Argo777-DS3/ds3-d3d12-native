#pragma once

#include <bit>
#include <cmath>
#include <cstdint>

namespace i0014::trtf {

constexpr float kReferenceDelta = 0.0166666005f;
constexpr float kStateMinimum = -0.5f;
constexpr float kStateMaximum = 0.5f;

enum class Branch : std::uint32_t {
    invalid = 0,
    native_reference = 1,
    additive = 2,
    decay = 3,
    reset = 4,
    clamp = 5,
};

struct Input {
    float delta = 0.0f;
    float state_before = 0.0f;
    float native_state = 0.0f;
    float retention = 0.0f;
};

struct Decision {
    bool valid = false;
    bool write = false;
    Branch branch = Branch::invalid;
    float final_state = 0.0f;
    float step_ratio = 1.0f;
};

inline Decision Evaluate(const Input& input) {
    Decision decision{};
    decision.final_state = input.native_state;
    if (!std::isfinite(input.delta) || input.delta <= 0.0f ||
        input.delta >= 0.1f || !std::isfinite(input.state_before) ||
        !std::isfinite(input.native_state) ||
        !std::isfinite(input.retention)) {
        return decision;
    }

    decision.valid = true;
    if (std::bit_cast<std::uint32_t>(input.delta) ==
        std::bit_cast<std::uint32_t>(kReferenceDelta)) {
        decision.branch = Branch::native_reference;
        return decision;
    }
    if (input.native_state == 0.0f) {
        decision.branch = Branch::reset;
        return decision;
    }
    if (std::fabs(input.native_state) == kStateMaximum) {
        decision.branch = Branch::clamp;
        return decision;
    }

    const double ratio = static_cast<double>(input.delta) /
                         static_cast<double>(kReferenceDelta);
    const bool decay =
        std::bit_cast<std::uint32_t>(input.native_state) ==
        std::bit_cast<std::uint32_t>(input.state_before * input.retention);
    float corrected = input.native_state;
    if (decay && input.retention > 0.0f && input.retention <= 1.0f) {
        corrected = static_cast<float>(
            static_cast<double>(input.state_before) *
            std::pow(static_cast<double>(input.retention), ratio));
        decision.branch = Branch::decay;
    } else {
        corrected = static_cast<float>(
            static_cast<double>(input.state_before) +
            static_cast<double>(input.native_state - input.state_before) *
                ratio);
        decision.branch = Branch::additive;
    }
    if (corrected > kStateMaximum) corrected = kStateMaximum;
    if (corrected < kStateMinimum) corrected = kStateMinimum;
    if (!std::isfinite(corrected)) {
        decision.valid = false;
        decision.branch = Branch::invalid;
        return decision;
    }
    decision.write = true;
    decision.final_state = corrected;
    decision.step_ratio = static_cast<float>(ratio);
    return decision;
}

}  // namespace i0014::trtf
