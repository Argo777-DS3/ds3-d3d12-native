#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "control_core.hpp"
#include "timing_core.hpp"
#include "trtf_core.hpp"
#include "../../I-0010/prototype/prototype_core.hpp"
#include "../../I-0010/prototype/debug_config.hpp"

namespace {

int g_failures = 0;

void Check(bool condition, const char* label) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        ++g_failures;
    }
}

void TestCadenceModel() {
    const i0014::TimingWritePlan native = i0014::ResolveCadenceRequest(
        {i0014::CadenceMode::native, 0.0f}, 30.0f, 0);
    Check(native.supported && !native.use_override && native.fps == 30.0f &&
          native.enable == 0, "native restores exact pair");

    const i0014::TimingWritePlan numeric = i0014::ResolveCadenceRequest(
        {i0014::CadenceMode::numeric, 165.0f}, 30.0f, 0);
    Check(numeric.supported && numeric.use_override && numeric.fps == 165.0f &&
          numeric.enable == 1, "numeric uses single finite state");

    const i0014::TimingWritePlan unlimited = i0014::ResolveCadenceRequest(
        {i0014::CadenceMode::unlimited, 0.0f}, 30.0f, 0);
    Check(!unlimited.supported, "unlimited remains genuinely unsupported");
    Check(!i0014::IsNumericFps(29.0f) && i0014::IsNumericFps(360.0f) &&
          !i0014::IsNumericFps(361.0f), "numeric range is bounded");
}

void TestAcceptedComparison() {
    DistanceFrame previous{};
    previous.index = 41;
    previous.delta = 1.0f / 165.0f;
    previous.transform = 0x12340000;
    previous.position[0] = 1.0f;
    previous.position[1] = 2.0f;
    previous.position[2] = 3.0f;
    previous.valid = true;

    DistanceFrame current = previous;
    current.index = 42;
    current.position[0] = 1.01f;

    ComparisonGuardInput input{};
    input.opcode[0] = 0x0F;
    input.opcode[1] = 0x2F;
    input.opcode[2] = 0xF8;
    input.previous = previous;
    input.current = current;
    input.expected_index = 42;
    input.expected_transform = previous.transform;
    std::memcpy(input.owner_position, current.position, 12);
    std::memcpy(input.getter_position, current.position, 12);
    std::memcpy(input.old_position, previous.position, 12);
    input.raw = 0.5f;

    ComparisonDecision decision{};
    Check(EvaluateComparison(input, &decision), "accepted guard identity");
    const float expected = input.raw * ((1.0f / 60.0f) / previous.delta);
    Check(std::memcmp(&decision.normalized, &expected, sizeof(float)) == 0,
          "accepted formula is bit-identical");
    Check(decision.native_reduces && !decision.normalized_reduces,
          "high-cadence decision changes only through normalization");

    input.opcode[0] = 0x90;
    Check(!EvaluateComparison(input, &decision), "opcode mismatch fails closed");
}

void TestAcceptedTerrainResponse() {
    using i0014::trtf::Branch;
    using i0014::trtf::Decision;
    using i0014::trtf::Input;

    const Decision native = i0014::trtf::Evaluate(
        {i0014::trtf::kReferenceDelta, 0.1f, 0.3f, 0.6f});
    const float native_expected = 0.3f;
    Check(native.valid && !native.write &&
          native.branch == Branch::native_reference &&
          std::memcmp(&native.final_state, &native_expected,
                      sizeof(float)) == 0,
          "TRTF native reference is bit-exact no-write passthrough");

    const Input additive_input{1.0f / 240.0f, 0.1f, 0.3f, 0.6f};
    const Decision additive = i0014::trtf::Evaluate(additive_input);
    const double additive_ratio =
        static_cast<double>(additive_input.delta) /
        static_cast<double>(i0014::trtf::kReferenceDelta);
    const float additive_expected = static_cast<float>(
        static_cast<double>(additive_input.state_before) +
        static_cast<double>(additive_input.native_state -
                            additive_input.state_before) * additive_ratio);
    Check(additive.valid && additive.write &&
          additive.branch == Branch::additive &&
          std::memcmp(&additive.final_state, &additive_expected,
                      sizeof(float)) == 0,
          "TRTF additive law is exact");

    const float decay_before = 0.4f;
    const float retention = 0.6f;
    const float native_decay = decay_before * retention;
    const Input decay_input{1.0f / 240.0f, decay_before, native_decay,
                            retention};
    const Decision decay = i0014::trtf::Evaluate(decay_input);
    const double decay_ratio =
        static_cast<double>(decay_input.delta) /
        static_cast<double>(i0014::trtf::kReferenceDelta);
    const float decay_expected = static_cast<float>(
        static_cast<double>(decay_before) *
        std::pow(static_cast<double>(retention), decay_ratio));
    Check(decay.valid && decay.write && decay.branch == Branch::decay &&
          std::memcmp(&decay.final_state, &decay_expected,
                      sizeof(float)) == 0,
          "TRTF multiplicative decay law is exact");

    const Decision reset = i0014::trtf::Evaluate(
        {1.0f / 240.0f, 0.1f, 0.0f, retention});
    const Decision clamp = i0014::trtf::Evaluate(
        {1.0f / 240.0f, 0.4f, 0.5f, retention});
    Check(reset.valid && !reset.write && reset.branch == Branch::reset,
          "TRTF reset remains native passthrough");
    Check(clamp.valid && !clamp.write && clamp.branch == Branch::clamp,
          "TRTF clamp remains native passthrough");
}

void TestIndependentCorrectionLifecycle() {
    i0014::CorrectionControlState state{};
    Check(!state.ltf_active && !state.trtf_active,
          "both corrections start OFF");

    i0014::ApplyCorrectionState(&state, i0014::CorrectionKind::ltf, true);
    Check(state.ltf_active && !state.trtf_active,
          "LTF ON does not alter TRTF");
    i0014::ApplyCorrectionState(&state, i0014::CorrectionKind::ltf, false);
    Check(!state.ltf_active && !state.trtf_active,
          "LTF OFF does not alter TRTF");
    i0014::ApplyCorrectionState(&state, i0014::CorrectionKind::ltf, true);

    i0014::ApplyCorrectionState(&state, i0014::CorrectionKind::trtf, true);
    i0014::MarkTrtfStateWrite(&state);
    Check(state.ltf_active && state.trtf_active && state.trtf_state_dirty,
          "LTF and TRTF coexist with distinct state");
    i0014::ApplyCorrectionState(&state, i0014::CorrectionKind::trtf, false);
    Check(state.ltf_active && !state.trtf_active && state.trtf_state_dirty,
          "TRTF OFF preserves LTF and requires its own restoration");
    state.trtf_state_dirty = false;
    i0014::ApplyCorrectionState(&state, i0014::CorrectionKind::trtf, true);
    const i0014::CorrectionRestorePlan plan =
        i0014::PlanCorrectionRestore(state);
    Check(plan.disable_ltf && plan.disable_trtf &&
          !plan.restore_trtf_state,
          "stop plan disables both without inventing shared state");
    Check(state.ltf_transitions == 3 && state.trtf_transitions == 3,
          "each correction completed ON-OFF-ON independently");
}

void TestFpsWindowAndDebugPlan() {
    i0014::FpsWindow window{};
    Check(!window.Add(1000, 1000), "fps window initializes");
    for (int index = 1; index <= 30; ++index) {
        window.Add(1000 + index * 10, 1000);
    }
    Check(!window.Add(1499, 1000), "fps window waits for half second");
    Check(window.Add(1500, 1000) && window.last_fps > 50.0f,
          "fps window publishes measured cadence");

    constexpr DWORD64 original = 0x12345678000000AAull;
    const DWORD64 configured = ConfigurePrototypeDr7(original);
    Check((configured & 0x3Full) == 0x15ull,
          "three local debug slots enabled");
    Check(configured != original, "debug plan is explicit and reversible");
}

}  // namespace

int main() {
    TestCadenceModel();
    TestAcceptedComparison();
    TestAcceptedTerrainResponse();
    TestIndependentCorrectionLifecycle();
    TestFpsWindowAndDebugPlan();
    const i0014::RuntimeStatus status{};
    Check(status.state == i0014::RuntimeState::starting &&
          !status.ltf_active && !status.trtf_active &&
          status.timing_restored && status.debug_restored,
          "public status starts inert and restored");
    if (g_failures != 0) return 1;
    std::puts("PASS: independent LTF/TRTF formulas and ON-OFF-ON lifecycles, "
              "coexistence, cadence model, and inert/restored state.");
    return 0;
}
