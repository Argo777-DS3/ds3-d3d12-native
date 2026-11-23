// The native D3D12 module loaded into DarkSoulsIII: it hooks the game's renderer entries and draws every frame with the native
// backend (native mode); Reference mode observes the original's frames in a separate process. Its body is in game_bootstrap/*.inc.
#include "game_bootstrap_includes.h"

// The module is one translation unit: the parts below share one anonymous namespace (state, hooks, collect, prepare,
// set-up), each a responsibility, in definition order: a part uses only what the earlier parts define, and includes the
// part before it (once only) so that clangd can check it on its own. One unit keeps the inlining and the static
// initialisation order of the former single file (code cleanup D-0191: the split changed no machine code, as
// scripts/Compare-NativeBinary.ps1 -Objects shows against the former object).
#include "game_bootstrap/01_state.inc"
#include "game_bootstrap/02_frame_lifecycle.inc"
#include "game_bootstrap/02b_rekindled_graphics.inc"
#include "game_bootstrap/03_engine_cpu_owners.inc"
#include "game_bootstrap/04_updates_and_test_tools.inc"
#include "game_bootstrap/04b_pass_witnesses.inc"
#include "game_bootstrap/04c_scene_witnesses.inc"
#include "game_bootstrap/05_selected_scene.inc"
#include "game_bootstrap/06_pass_programmes.inc"
#include "game_bootstrap/07_scene_lighting.inc"
#include "game_bootstrap/08_model_programmes.inc"
#include "game_bootstrap/09_model_admission.inc"
#include "game_bootstrap/10_options_and_passes.inc"
#include "game_bootstrap/12_scene_collect.inc"
#include "game_bootstrap/13_model_frame_inputs.inc"
#include "game_bootstrap/13b_prepare_model_frame.inc"
#include "game_bootstrap/14_scene_step_and_storage.inc"
#include "game_bootstrap/15_cpu_service_and_input.inc"
#include "game_bootstrap/16_offscreen_and_movies.inc"
#include "game_bootstrap/16b_rekindled_overlay.inc"
#include "game_bootstrap/17_render_step.inc"
#include "game_bootstrap/18_resource_admission.inc"
#include "game_bootstrap/19_ui_witnesses.inc"
#include "game_bootstrap/20_startup_phases.inc"
#include "game_bootstrap/21_effects.inc"
#include "game_bootstrap/22_setup.inc"
BOOL WINAPI DllMain(HINSTANCE module,DWORD reason,void* reserved) {
    if(reason==DLL_PROCESS_DETACH && reserved)native_process_terminating.store(true,std::memory_order_release);
    if(reason==DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        HANDLE worker=CreateThread(nullptr,0,Setup,nullptr,0,nullptr);
        if(!worker) return FALSE;
        CloseHandle(worker);
    }
    return TRUE;
}
