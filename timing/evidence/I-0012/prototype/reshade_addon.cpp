#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <imgui.h>
#include <reshade.hpp>

#include <algorithm>

#include "timing_core.hpp"

namespace {

int g_selected_fps = 165;

const char* ResultName(i0012::ControlResult result) {
    switch (result) {
        case i0012::ControlResult::none: return "none";
        case i0012::ControlResult::queued: return "queued";
        case i0012::ControlResult::applied: return "applied";
        case i0012::ControlResult::invalid: return "invalid";
        case i0012::ControlResult::unsupported: return "unsupported";
        case i0012::ControlResult::failed: return "failed";
    }
    return "unknown";
}

void DrawSettings(reshade::api::effect_runtime*) {
    const i0012::RuntimeStatus status = i0012::GetStatus();
    const bool operable = status.ready &&
        status.state != i0012::RuntimeState::failed &&
        status.state != i0012::RuntimeState::stopping &&
        status.state != i0012::RuntimeState::stopped;

    ImGui::Text("Core: %s", i0012::RuntimeStateName(status.state));
    ImGui::Text("Actual: %.1f FPS", status.actual_fps);
    if (status.cadence_mode == i0012::CadenceMode::numeric) {
        ImGui::Text("Target: %.0f FPS", status.target_fps);
    } else {
        ImGui::TextUnformatted("Target: Native");
    }
    ImGui::Text("Last control: %s", ResultName(status.last_control_result));

    if (status.guard_failed) {
        ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                           "Guard failure: core stopped closed.");
    }

    ImGui::Separator();
    ImGui::BeginDisabled(!operable);
    ImGui::SliderInt("FPS", &g_selected_fps,
                     static_cast<int>(i0012::kMinimumFps),
                     static_cast<int>(i0012::kMaximumFps));
    if (ImGui::Button("Apply numeric")) {
        i0012::RequestNumericCadence(static_cast<float>(g_selected_fps));
    }
    ImGui::SameLine();
    if (ImGui::Button("Native")) {
        i0012::RequestNativeCadence();
    }

    for (std::size_t index = 0; index < i0012::kFpsPresets.size(); ++index) {
        if (index != 0) ImGui::SameLine();
        const int preset = i0012::kFpsPresets[index];
        char label[16]{};
        _snprintf_s(label, _countof(label), _TRUNCATE, "%d", preset);
        if (ImGui::SmallButton(label)) {
            g_selected_fps = preset;
            i0012::RequestNumericCadence(static_cast<float>(preset));
        }
    }

    bool correction = status.correction_active;
    if (ImGui::Checkbox("Locomotion timing correction", &correction)) {
        i0012::RequestCorrection(correction);
    }
    ImGui::SetItemTooltip(
        "Accepted cadence-independent comparison correction; no actor table.");

    ImGui::BeginDisabled();
    ImGui::Button("Unlimited unavailable");
    ImGui::EndDisabled();
    ImGui::SetItemTooltip(
        "No genuine limiter-off engine state has been established.");

    if (ImGui::Button("Restore Native & Stop")) {
        i0012::RequestStop();
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::Text("Updates: %llu | comparisons: %llu | corrected: %llu",
                static_cast<unsigned long long>(status.updates),
                static_cast<unsigned long long>(status.comparisons),
                static_cast<unsigned long long>(status.corrections));
    ImGui::Text("Restored: timing %s | debug %s",
                status.timing_restored ? "yes" : "no",
                status.debug_restored ? "yes" : "no");
    ImGui::TextDisabled(
        "Test interface only. Starts inert; changes apply on an update boundary.");
}

}  // namespace

extern "C" __declspec(dllexport) const char* NAME =
    "DS3 Timing Validation";
extern "C" __declspec(dllexport) const char* DESCRIPTION =
    "Exact-build cadence and locomotion timing controls for bounded testing.";

extern "C" __declspec(dllexport) void AddonUninit(HMODULE, HMODULE) {
    if (!i0012::StopAndWait(5000)) {
        HMODULE pinned = nullptr;
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_PIN |
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<LPCWSTR>(&AddonUninit), &pinned);
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(module);
            if (!reshade::register_addon(module)) return FALSE;
            i0012::Start(module);
            reshade::register_overlay(nullptr, DrawSettings);
            break;
        case DLL_PROCESS_DETACH:
            i0012::RequestStop();
            reshade::unregister_addon(module);
            break;
        default:
            break;
    }
    return TRUE;
}
