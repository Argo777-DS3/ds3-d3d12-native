#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

enum class RuntimeState : std::uint32_t {
    standby = 0,
    native = 1,
    active = 2,
    stopping = 3,
    failed = 4,
};

enum class CadenceMode : std::uint32_t {
    native = 0,
    numeric = 1,
    unlimited = 2,
};

enum class ControlResult : std::uint32_t {
    none = 0,
    applied = 1,
    invalid = 2,
    unsupported = 3,
    failed = 4,
};

struct CadenceRequest {
    CadenceMode mode = CadenceMode::native;
    float fps = 60.0f;
};

struct TimingWritePlan {
    bool supported = false;
    bool use_override = false;
    float fps = 0.0f;
    unsigned char enable = 0;
};

struct TimingPair {
    float fps = 0.0f;
    unsigned char enable = 0;
};

constexpr float kMinimumFps = 30.0f;
constexpr float kMaximumFps = 360.0f;
constexpr std::array<int, 7> kFpsPresets = {
    30, 60, 90, 120, 144, 165, 240,
};

inline float ClampSliderFps(float value) {
    if (!std::isfinite(value)) return kMinimumFps;
    return std::clamp(std::round(value), kMinimumFps, kMaximumFps);
}

inline bool IsNumericFps(float value) {
    return std::isfinite(value) && value >= kMinimumFps &&
           value <= kMaximumFps;
}

inline TimingWritePlan ResolveCadenceRequest(const CadenceRequest& request,
                                             float original_fps,
                                             unsigned char original_enable) {
    switch (request.mode) {
        case CadenceMode::native:
            return {true, false, original_fps, original_enable};
        case CadenceMode::numeric:
            if (!IsNumericFps(request.fps)) return {};
            return {true, true, request.fps, 1};
        case CadenceMode::unlimited:
            // The recovered pair has only native and finite numeric states.
            // No undocumented sentinel or artificial high cap is accepted.
            return {false, false, original_fps, original_enable};
    }
    return {};
}

inline bool ApplyTimingWritePlan(const TimingWritePlan& plan,
                                 TimingPair* pair) {
    if (!pair || !plan.supported || !std::isfinite(plan.fps)) return false;
    pair->enable = 0;
    pair->fps = plan.fps;
    pair->enable = plan.enable;
    return pair->fps == plan.fps && pair->enable == plan.enable;
}

inline RuntimeState StateForCadence(const CadenceRequest& request) {
    return request.mode == CadenceMode::numeric
        ? RuntimeState::active
        : RuntimeState::native;
}

struct FpsWindow {
    std::int64_t first_qpc = 0;
    std::uint32_t frames = 0;
    float last_fps = 0.0f;

    bool Add(std::int64_t qpc, std::int64_t frequency) {
        if (qpc <= 0 || frequency <= 0) return false;
        if (first_qpc == 0 || qpc <= first_qpc) {
            first_qpc = qpc;
            frames = 0;
            return false;
        }
        ++frames;
        const std::int64_t elapsed = qpc - first_qpc;
        if (elapsed < frequency / 2) return false;
        last_fps = static_cast<float>(
            static_cast<double>(frames) * static_cast<double>(frequency) /
            static_cast<double>(elapsed));
        first_qpc = qpc;
        frames = 0;
        return std::isfinite(last_fps) && last_fps >= 0.0f;
    }
};

struct IntRect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

inline bool Contains(const IntRect& rect, int x, int y) {
    return x >= rect.left && x < rect.right &&
           y >= rect.top && y < rect.bottom;
}

inline float SliderFpsFromX(int x, const IntRect& track) {
    if (track.right <= track.left) return kMinimumFps;
    const float fraction = std::clamp(
        static_cast<float>(x - track.left) /
            static_cast<float>(track.right - track.left),
        0.0f, 1.0f);
    return ClampSliderFps(
        kMinimumFps + fraction * (kMaximumFps - kMinimumFps));
}

inline int SliderXFromFps(float fps, const IntRect& track) {
    const float value = ClampSliderFps(fps);
    const float fraction = (value - kMinimumFps) /
                           (kMaximumFps - kMinimumFps);
    return track.left + static_cast<int>(std::lround(
        fraction * static_cast<float>(track.right - track.left)));
}

struct SliderDragState {
    bool active = false;
    float preview_fps = 60.0f;
};

inline void BeginSliderDrag(SliderDragState* state, int x,
                            const IntRect& track) {
    if (!state) return;
    state->active = true;
    state->preview_fps = SliderFpsFromX(x, track);
}

inline bool UpdateSliderDrag(SliderDragState* state, int x,
                             const IntRect& track) {
    if (!state || !state->active) return false;
    state->preview_fps = SliderFpsFromX(x, track);
    return true;
}

inline bool CommitSliderDrag(SliderDragState* state, int x,
                             const IntRect& track, float* committed_fps) {
    if (!state || !state->active || !committed_fps) return false;
    state->preview_fps = SliderFpsFromX(x, track);
    *committed_fps = state->preview_fps;
    state->active = false;
    return true;
}

inline void CancelSliderDrag(SliderDragState* state) {
    if (state) state->active = false;
}

inline IntRect SliderRect() { return {20, 124, 420, 142}; }
inline IntRect UnlimitedRect() { return {20, 236, 214, 270}; }
inline IntRect NativeRect() { return {226, 236, 420, 270}; }
inline IntRect FixRect() { return {20, 282, 214, 316}; }
inline IntRect StopRect() { return {226, 282, 420, 316}; }

inline IntRect PresetRect(std::size_t index) {
    constexpr int width = 94;
    constexpr int gap = 8;
    const int row = index < 4 ? 0 : 1;
    const int column = static_cast<int>(index < 4 ? index : index - 4);
    const int left = 20 + column * (width + gap);
    const int top = 158 + row * 40;
    return {left, top, left + width, top + 32};
}

enum class OverlayAction : std::uint32_t {
    none,
    slider,
    preset_30,
    preset_60,
    preset_90,
    preset_120,
    preset_144,
    preset_165,
    preset_240,
    unlimited,
    native,
    toggle_fix,
    stop,
};

inline OverlayAction HitTestOverlay(int x, int y) {
    if (Contains(SliderRect(), x, y)) return OverlayAction::slider;
    for (std::size_t index = 0; index < kFpsPresets.size(); ++index) {
        if (Contains(PresetRect(index), x, y)) {
            return static_cast<OverlayAction>(
                static_cast<std::uint32_t>(OverlayAction::preset_30) +
                static_cast<std::uint32_t>(index));
        }
    }
    if (Contains(UnlimitedRect(), x, y)) return OverlayAction::unlimited;
    if (Contains(NativeRect(), x, y)) return OverlayAction::native;
    if (Contains(FixRect(), x, y)) return OverlayAction::toggle_fix;
    if (Contains(StopRect(), x, y)) return OverlayAction::stop;
    return OverlayAction::none;
}

inline bool ShouldShowOverlay(bool requested, bool game_has_focus,
                              bool overlay_has_focus, bool game_minimized) {
    return requested && (game_has_focus || overlay_has_focus) &&
           !game_minimized;
}

inline bool OverlayPositionChanged(int current_x, int current_y,
                                   int desired_x, int desired_y) {
    return current_x != desired_x || current_y != desired_y;
}

inline const char* RuntimeStateName(RuntimeState state) {
    switch (state) {
        case RuntimeState::standby: return "STANDBY";
        case RuntimeState::native: return "NATIVE";
        case RuntimeState::active: return "ACTIVE";
        case RuntimeState::stopping: return "STOPPING";
        case RuntimeState::failed: return "FAILED";
    }
    return "UNKNOWN";
}
