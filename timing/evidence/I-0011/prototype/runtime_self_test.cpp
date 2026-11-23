#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

#include "runtime_core.hpp"
#include "../../I-0010/prototype/prototype_core.hpp"
#include "../../I-0010/prototype/debug_config.hpp"

namespace {

bool SameTimingPair(const TimingPair& left, const TimingPair& right) {
    return std::memcmp(&left.fps, &right.fps, sizeof(float)) == 0 &&
           left.enable == right.enable;
}

bool TestSliderAndPresets() {
    const IntRect track{20, 20, 350, 32};
    if (ClampSliderFps(-1.0f) != 30.0f ||
        ClampSliderFps(999.0f) != 360.0f ||
        ClampSliderFps(164.6f) != 165.0f ||
        SliderFpsFromX(track.left - 50, track) != 30.0f ||
        SliderFpsFromX(track.right + 50, track) != 360.0f ||
        SliderXFromFps(30.0f, track) != track.left ||
        SliderXFromFps(360.0f, track) != track.right) return false;
    SliderDragState drag{};
    float committed = 0.0f;
    if (UpdateSliderDrag(&drag, track.left, track) ||
        CommitSliderDrag(&drag, track.left, track, &committed)) return false;
    BeginSliderDrag(&drag, track.left, track);
    if (!drag.active || drag.preview_fps != 30.0f ||
        !UpdateSliderDrag(&drag, track.right, track) ||
        drag.preview_fps != 360.0f ||
        !CommitSliderDrag(&drag, (track.left + track.right) / 2,
                          track, &committed) ||
        drag.active || committed != 195.0f ||
        CommitSliderDrag(&drag, track.right, track, &committed)) return false;
    for (int preset : kFpsPresets) {
        const CadenceRequest request{CadenceMode::numeric,
                                     static_cast<float>(preset)};
        const TimingWritePlan plan = ResolveCadenceRequest(request, 30.0f, 0);
        if (!plan.supported || !plan.use_override ||
            plan.fps != static_cast<float>(preset) || plan.enable != 1) {
            return false;
        }
    }
    return true;
}

bool TestOverlayInteractionAndFocus() {
    if (HitTestOverlay(25, 130) != OverlayAction::slider ||
        HitTestOverlay(25, 165) != OverlayAction::preset_30 ||
        HitTestOverlay(125, 165) != OverlayAction::preset_60 ||
        HitTestOverlay(25, 242) != OverlayAction::unlimited ||
        HitTestOverlay(230, 242) != OverlayAction::native ||
        HitTestOverlay(25, 290) != OverlayAction::toggle_fix ||
        HitTestOverlay(230, 290) != OverlayAction::stop ||
        HitTestOverlay(430, 340) != OverlayAction::none) return false;
    return ShouldShowOverlay(true, true, false, false) &&
           ShouldShowOverlay(true, false, true, false) &&
           !ShouldShowOverlay(false, true, false, false) &&
           !ShouldShowOverlay(true, false, false, false) &&
           !ShouldShowOverlay(true, true, false, true) &&
           !OverlayPositionChanged(100, 200, 100, 200) &&
           OverlayPositionChanged(100, 200, 101, 200);
}

bool TestCadenceTransitionsAndRestoration() {
    const TimingPair original{30.0f, 0};
    TimingPair pair = original;
    const std::array<CadenceRequest, 4> sequence = {{
        {CadenceMode::numeric, 240.0f},
        {CadenceMode::unlimited, 0.0f},
        {CadenceMode::numeric, 165.0f},
        {CadenceMode::unlimited, 0.0f},
    }};
    unsigned int applied = 0;
    unsigned int unsupported = 0;
    for (const CadenceRequest& request : sequence) {
        const TimingPair before = pair;
        const TimingWritePlan plan = ResolveCadenceRequest(
            request, original.fps, original.enable);
        if (!plan.supported) {
            ++unsupported;
            if (!SameTimingPair(before, pair)) return false;
            continue;
        }
        if (!ApplyTimingWritePlan(plan, &pair)) return false;
        ++applied;
    }
    if (applied != 2 || unsupported != 2 ||
        pair.fps != 165.0f || pair.enable != 1) return false;
    const TimingWritePlan native = ResolveCadenceRequest(
        {CadenceMode::native, 0.0f}, original.fps, original.enable);
    return ApplyTimingWritePlan(native, &pair) &&
           SameTimingPair(pair, original);
}

bool TestTenRestorations() {
    const TimingPair original{30.0f, 0};
    for (int run = 0; run < 10; ++run) {
        TimingPair pair = original;
        const float fps = static_cast<float>(kFpsPresets[
            static_cast<std::size_t>(run) % kFpsPresets.size()]);
        if (!ApplyTimingWritePlan(
                ResolveCadenceRequest({CadenceMode::numeric, fps},
                                      original.fps, original.enable), &pair) ||
            !ApplyTimingWritePlan(
                ResolveCadenceRequest({CadenceMode::native, 0.0f},
                                      original.fps, original.enable), &pair) ||
            !SameTimingPair(pair, original)) return false;
    }
    return true;
}

bool TestActualFps() {
    FpsWindow window{};
    constexpr std::int64_t frequency = 1000000;
    constexpr std::int64_t interval = frequency / 120;
    std::int64_t now = 1000000;
    window.Add(now, frequency);
    bool published = false;
    for (int i = 0; i < 121; ++i) {
        now += interval;
        published = window.Add(now, frequency) || published;
    }
    return published && window.last_fps > 119.0f &&
           window.last_fps < 121.0f;
}

bool TestAcceptedFormulaAndGuards() {
    ComparisonGuardInput input{};
    input.opcode[0] = 0x0F;
    input.opcode[1] = 0x2F;
    input.opcode[2] = 0xF8;
    input.previous = DistanceFrame{10, 1.0f / 240.0f, 0x1234,
                                   {1.0f, 2.0f, 3.0f}, true};
    input.current = DistanceFrame{11, 1.0f / 240.0f, 0x1234,
                                  {2.0f, 3.0f, 4.0f}, true};
    input.expected_index = 11;
    input.expected_transform = 0x1234;
    std::memcpy(input.owner_position, input.current.position, 12);
    std::memcpy(input.getter_position, input.current.position, 12);
    std::memcpy(input.old_position, input.previous.position, 12);
    input.raw = 0.5f;
    ComparisonDecision decision{};
    if (!EvaluateComparison(input, &decision) ||
        std::fabs(decision.normalized - 2.0f) > 1e-6f ||
        !decision.native_reduces || decision.normalized_reduces) return false;
    input.opcode[0] = 0x90;
    if (EvaluateComparison(input, &decision)) return false;
    input.opcode[0] = 0x0F;
    input.previous.delta = std::numeric_limits<float>::infinity();
    return !EvaluateComparison(input, &decision);
}

bool TestDebugConfigurationRestoration() {
    CONTEXT original{};
    original.Dr0 = 1;
    original.Dr1 = 2;
    original.Dr2 = 3;
    original.Dr3 = 4;
    original.Dr6 = 5;
    original.Dr7 = 0xFFFF;
    CONTEXT armed = original;
    armed.Dr7 = ConfigurePrototypeDr7(armed.Dr7);
    CONTEXT restored = armed;
    restored.Dr0 = original.Dr0;
    restored.Dr1 = original.Dr1;
    restored.Dr2 = original.Dr2;
    restored.Dr3 = original.Dr3;
    restored.Dr6 = original.Dr6;
    restored.Dr7 = original.Dr7;
    return SameDebugState(original, restored);
}

}  // namespace

int main() {
    if (!TestSliderAndPresets()) return 1;
    if (!TestOverlayInteractionAndFocus()) return 2;
    if (!TestCadenceTransitionsAndRestoration()) return 3;
    if (!TestTenRestorations()) return 4;
    if (!TestActualFps()) return 5;
    if (!TestAcceptedFormulaAndGuards()) return 6;
    if (!TestDebugConfigurationRestoration()) return 7;
    std::puts("PASS: overlay hit/focus/placement logic, commit-on-release slider/presets, single cadence path, explicit Unlimited rejection, actual-FPS meter, unchanged formula/guards and ten exact restorations.");
    return 0;
}
