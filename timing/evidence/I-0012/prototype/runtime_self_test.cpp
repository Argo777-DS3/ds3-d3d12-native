#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "control_core.hpp"
#include "timing_core.hpp"
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
    const i0012::TimingWritePlan native = i0012::ResolveCadenceRequest(
        {i0012::CadenceMode::native, 0.0f}, 30.0f, 0);
    Check(native.supported && !native.use_override && native.fps == 30.0f &&
          native.enable == 0, "native restores exact pair");

    const i0012::TimingWritePlan numeric = i0012::ResolveCadenceRequest(
        {i0012::CadenceMode::numeric, 165.0f}, 30.0f, 0);
    Check(numeric.supported && numeric.use_override && numeric.fps == 165.0f &&
          numeric.enable == 1, "numeric uses single finite state");

    const i0012::TimingWritePlan unlimited = i0012::ResolveCadenceRequest(
        {i0012::CadenceMode::unlimited, 0.0f}, 30.0f, 0);
    Check(!unlimited.supported, "unlimited remains genuinely unsupported");
    Check(!i0012::IsNumericFps(29.0f) && i0012::IsNumericFps(360.0f) &&
          !i0012::IsNumericFps(361.0f), "numeric range is bounded");
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

void TestFpsWindowAndDebugPlan() {
    i0012::FpsWindow window{};
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
    TestFpsWindowAndDebugPlan();
    const i0012::RuntimeStatus status{};
    Check(status.state == i0012::RuntimeState::starting &&
          status.timing_restored && status.debug_restored,
          "public status starts inert and restored");
    if (g_failures != 0) return 1;
    std::puts("PASS: independent cadence core, accepted formula/guards, "
              "measured-FPS window and inert/restored state.");
    return 0;
}
