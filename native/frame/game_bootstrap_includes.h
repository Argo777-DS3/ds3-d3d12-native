// The includes of game_bootstrap.cpp's translation unit (code cleanup D-0191): the parts under game_bootstrap/ include
// it first, so that each part can be read and checked on its own (clangd).
#pragma once
#include "collect_profile.h"
#include "frame_store.h"
#include "ffx_observe.h"
#include "effect_system.h"
#include "effect_draw.h"
#include "shadow_state.h"
#include "game_bootstrap_shared.h"
#include <psapi.h>
#include <tlhelp32.h>   // I-0078: the original probe's main thread
#include <dwmapi.h>     // I-0132 R12h: the game window's dark title bar
#include <wincodec.h>   // I-0132 R12h's second tranche: the precompile page's picture (WIC)
#include <mmsystem.h>   // and its sound (waveOut)
#include <mfapi.h>      // its music decoded by Media Foundation (R12h's third tranche)
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include "game_texture_ingress.h"
#include "game_quality_object.h"
#include "gx_mtd_image.h"
#include "display_blobs.h"
namespace ds3content {bool Sha256(const void* data,size_t bytes,char (&hex)[65]) noexcept;}   // W3: content/acquire.cpp (uppercase hex)
#include "engine_window.h"
#include "primitive_programs.h"
#include "primitive_constants.h"
#include "game_primitive_mesh_source.h"
#include "game_primitive_image_source.h"
#include "game_ui_image_owner.h"
#include "game_ui_shaders.h"
#include "game_ui_primitive.h"
#include "game_ui_mesh_owner.h"
#include "game_ui_session.h"
#include "scene_ingress.h"
#include "game_scene_geometry.h"
#include "game_model_selection.h"
#include "simulation_time.h"
#include "../renderer/scene_pass.h"
#include "../renderer/effect_scan.h"   // I-0113 tranche 30: the Reference scan
#include "../renderer/raytracing_base.h"
#include "../renderer/rt_scene.h"
#include "../renderer/rt_light_grid.h"   // I-0128 R8c
#include "../renderer/render_worker.h"
#include "../renderer/cadence_trace.h"   // I-0097 tranche 5 (cadence-trace.flag)
#include "../renderer/model_geometry.h"
#include "../renderer/velocity.h"
#include "../renderer/upscaler.h"         // I-0111 (R1)
#include "../renderer/streamline_host.h"
#include "../renderer/glow_math.h"
#include "../renderer/tess_constants.h"
#include "../renderer/overlay_text.h"   // I-0132 R12a: the overlay's text in DS3's fonts
#include "../third_party/zlib/zlib.h"   // I-0132 R12a: a « CFX » font movie inflated
#include "../content/content_hash.h"   // I-0097 tranche 4 (D-0211): the native cache keys
#include "../content/gparam.h"
#include "../content/model_programs.h"
#include "../content/model_textures.h"
#include "../content/model_common_textures.h"
#include "../content/model_parameters.h"
#include "../content/decal_constants.h"
#include "../content/paint_constants.h"
#include "../content/shader_constants.h"
#include "../content/shader_reads.h"
#include "../content/sampler_bank.h"
#include "../content/model_vertex_layout.h"
#include "../content/model_pose.h"
#include "../content/model_palette_indices.h"
#include "../content/model_pose_binding.h"
#include "../content/model_pass.h"
#include "../content/texture_uv_scope.h"
#include "../world/host_input.h"
#include "../world/light_producers.h"
#include "../world/light_tiles.h"
#include "../world/yebis_blocks.h"
#include "../world/yebis_tonemap.h"
#include "../world/post_tone.h"
#include "../renderer/post_glare_programs.h"
#include "../world/menu_tonemap.h"   // I-0113 tranche 27
#include "../world/yebis_shafts.h"
#include "../world/yebis_dof.h"
#include "../world/yebis_motionblur.h"
#include "../world/producers.h"
#include "../world/env_region.h"
#include "../world/sort_key.h"
#include "sample_profile.h"
#include "crash_dump.h"                     // I-0082
#include <csignal>
#include "../renderer/device_removal.h"     // I-0082
#include <MinHook.h>
#include <intrin.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <deque>
#include <vector>
#include <filesystem>
#include <fstream>
#include <optional>
#include <list>
#include <condition_variable>
#include <thread>
#include <map>
#include <set>
#include <tuple>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <d3d11.h>
