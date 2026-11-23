#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <imgui.h>
#include <reshade.hpp>

#include <algorithm>

#include "timing_core.hpp"

namespace {

int g_selected_fps = 60;

const char* ResultName(i0014::ControlResult result) {
    switch (result) {
        case i0014::ControlResult::none: return "none";
        case i0014::ControlResult::queued: return "queued";
        case i0014::ControlResult::applied: return "applied";
        case i0014::ControlResult::invalid: return "invalid";
        case i0014::ControlResult::unsupported: return "unsupported";
        case i0014::ControlResult::failed: return "failed";
    }
    return "unknown";
}

void DrawSettings(reshade::api::effect_runtime*) {
    const i0014::RuntimeStatus status = i0014::GetStatus();
    const bool operable = status.ready &&
        status.state != i0014::RuntimeState::failed &&
        status.state != i0014::RuntimeState::stopping &&
        status.state != i0014::RuntimeState::stopped;

    ImGui::Text("Core: %s", i0014::RuntimeStateName(status.state));
    ImGui::Text("Actual: %.1f FPS", status.actual_fps);
    if (status.cadence_mode == i0014::CadenceMode::numeric) {
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
                     static_cast<int>(i0014::kMinimumFps),
                     static_cast<int>(i0014::kMaximumFps));
    if (ImGui::Button("Apply numeric")) {
        i0014::RequestNumericCadence(static_cast<float>(g_selected_fps));
    }
    ImGui::SameLine();
    if (ImGui::Button("Native")) {
        i0014::RequestNativeCadence();
    }

    for (std::size_t index = 0; index < i0014::kFpsPresets.size(); ++index) {
        if (index != 0) ImGui::SameLine();
        const int preset = i0014::kFpsPresets[index];
        char label[16]{};
        _snprintf_s(label, _countof(label), _TRUNCATE, "%d", preset);
        if (ImGui::SmallButton(label)) {
            g_selected_fps = preset;
            i0014::RequestNumericCadence(static_cast<float>(preset));
        }
    }

    bool ltf = status.ltf_active;
    if (ImGui::Checkbox("LTF — Locomotion Timing Fix", &ltf)) {
        i0014::RequestLtf(ltf);
    }
    ImGui::SetItemTooltip(
        "Independent accepted locomotion comparison correction.");

    bool trtf = status.trtf_active;
    if (ImGui::Checkbox("TRTF — Terrain Response Timing Fix", &trtf)) {
        i0014::RequestTrtf(trtf);
    }
    ImGui::SetItemTooltip(
        "Independent accepted terrain-response recurrence correction.");

    ImGui::BeginDisabled();
    ImGui::Button("Unlimited unavailable");
    ImGui::EndDisabled();
    ImGui::SetItemTooltip(
        "No genuine limiter-off engine state has been established.");

    if (ImGui::Button("Restore Native + Fixes OFF")) {
        i0014::RequestLtf(false);
        i0014::RequestTrtf(false);
        i0014::RequestNativeCadence();
    }
    ImGui::SameLine();
    if (ImGui::Button("Restore Native & Stop")) {
        i0014::RequestStop();
    }
    ImGui::EndDisabled();

    ImGui::Separator();
    ImGui::Text("Updates: %llu | LTF compared: %llu | LTF corrected: %llu",
                static_cast<unsigned long long>(status.updates),
                static_cast<unsigned long long>(status.ltf_comparisons),
                static_cast<unsigned long long>(status.ltf_corrections));
    ImGui::Text("TRTF calls: %llu | writes: %llu | native passthrough: %llu",
                static_cast<unsigned long long>(status.trtf_calls),
                static_cast<unsigned long long>(status.trtf_writes),
                static_cast<unsigned long long>(
                    status.trtf_native_passthroughs));
    ImGui::Text("Restored: timing %s | debug %s",
                status.timing_restored ? "yes" : "no",
                status.debug_restored ? "yes" : "no");
    ImGui::TextDisabled(
        "Test interface only. Starts inert; changes apply on an update boundary.");
    ImGui::TextDisabled(
        "Fullscreen keys: Ctrl+F6 LTF, Ctrl+F7 TRTF, Ctrl+F8 restore all.");
    ImGui::TextDisabled(
        "Ctrl+F9 60 FPS, Ctrl+F10 240 FPS, Ctrl+F11 Native, Ctrl+F12 Stop.");
}

}  // namespace

extern "C" __declspec(dllexport) const char* NAME =
    "DS3 Unified Timing Fix Test";
extern "C" __declspec(dllexport) const char* DESCRIPTION =
    "Independent exact-build LTF/TRTF and cadence controls for bounded testing.";

extern "C" __declspec(dllexport) void AddonUninit(HMODULE, HMODULE) {
    if (!i0014::StopAndWait(5000)) {
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
            i0014::Start(module);
            reshade::register_overlay(nullptr, DrawSettings);
            break;
        case DLL_PROCESS_DETACH:
            i0014::RequestStop();
            reshade::unregister_addon(module);
            break;
        default:
            break;
    }
    return TRUE;
}
