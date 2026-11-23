# I-0120's low findings located in today's code (2026-11-12)

A Sonnet 5.5 subagent's locating pass (read-only) over `main` `4cab0003`: for each finding of
[I-0120's REVIEW](../I-0120-full-code-review/REVIEW.md) that I-0121 left, where its code is today and whether it still does what
the finding says. **Worker output condensed by the pilot, not checked**: each row is re-read in the code before it is corrected,
and the RESULT records what was found then. GB = `native/frame/game_bootstrap`, R = `native/renderer`, F = `native/frame`.

| # | Today | State | Note |
|---|---|---|---|
| A4 | GB/01_state.inc (the statics `textures`, `model_storage`, `cloth_streams`, `native_common_images`, `native_stage_textures`), 02:179, 201, 03:790, 989 | present | `native_process_terminating` exists since; no static owning D3D12 objects is guarded by it |
| A6 | GB/17_render_step.inc:142-154 | present | the hash every upload, read until `simulated` is sticky; I-0134 added `previous_resource` there |
| A7 | GB/03_engine_cpu_owners.inc:1089, 1120 (catches 1098, 1155) | present | the cloth path returns quietly (17:141-150) |
| A8 | GB/01_state.inc:160, 168; 03:700-701, 747-748 | present | written, never read |
| A9 | GB/01_state.inc:470; written 14:409; read 09:495-507 | present | nothing erases `model_geometry_refusal` |
| B4 | GB/05_selected_scene.inc:72-114 | present | compose, scan, copy and Log inside the lock |
| B5 | GB/13_model_frame_inputs.inc:176-188; 13b:136-148 | present | |
| B8 | GB/12_scene_collect.inc:3018; 16:427; F/game_model_selection.cpp:67-77 | present | read every collect, two callers |
| B9 | F/game_scene_geometry.cpp:92-107 | present | `vertex_offset2`, `stride2` not compared; no refusal code |
| B10 | R/model_frame.h:21-22 | changed | only « no point/spot lights yet » left |
| B11 | F/game_model_selection.cpp:98-100 | present | |
| C5 | GB/08_model_programmes.inc:8-80, 351-369 | present | the « no effect owns it » answer is cached since I-0100; a failed composition is not |
| C6 | GB/08_model_programmes.inc:388, 462-476 | present | |
| C7 | native/content/decal_constants.cpp:35 | present | `0xad49cbc5`; the game's word `0xAD49CBA5` |
| C9 | GB/06_pass_programmes.inc:357-367 | present | callers 13b:1465, 1514, 1577 |
| C10 | - | removed | the raster reflection collect is gone (D-0251); stale comments at 08:1444, 09:647 |
| D3 | F/effect_draw.cpp:472-485; GB/17_render_step.inc:84-110 | present | |
| D4 | GB/12_scene_collect.inc:694-715, 734; 07:752-785 | present | |
| D6 | native/world/light_producers.cpp (Snapshots, Produce, ProduceKeyed, Banks::Env, EnvIds, SssBase, SssInstance, BankCheck; the three .txt reads 1178-1186, the MapOffset fallbacks 1298, 1355); light_tiles.cpp:72 `View::From` | present | `GroupWord` used by a test only; `BackgroundColor` and `EnvColorScale` are production |
| D7 | native/world/host_input.h:49-68 | present | |
| E4 | R/scene_pass_views.cpp:152-158; scene_pass_velocity.cpp:407-473 | present | |
| E5 | R/scene_pass.cpp:95-97; scene_pass.h:738, 740 | present | |
| E6 | R/scene_pass.cpp:130, 334; scene_pass.h:706 | present | |
| E7 | R/frame_plan.cpp:7-104; frame_graph.cpp:98; scene_pass_frame.cpp:36 | present | `MaterialColorFramePlan`, `PlanFramePlacement`, `TakePlanPassesPeak` in `frame_graph_test.cpp` only; `RenderNativeModels` no caller |
| E8 | R/scene_pass_frame.cpp:1766 | changed | the quoted comments are gone; `yebis_stage.h:12` still says « Inert until YB-8/YB-9 » |
| F2 | GB/14_scene_step_and_storage.inc:261-264 | present | |
| F3 | R/rt_scene.cpp:333 | present | |
| F4 | R/grading_stage.cpp:128-132 | present | |
| F5 | R/yebis_stage.h:12-19; GB/06_pass_programmes.inc:186-203, 237, 256 | present | `scene_copy`, `dof_table` assigned, never read |
| F6 | - | removed | the SSAO stage is gone (I-0126 R7c) |
| F7 | R/tonemap_stage.h:21, 43 | present | |
| F8 | R/rt_scene.h:4-5 | changed | « nothing here draws » still there |
| G2 | F/game_ui_bindings.cpp:69-71; game_ui_session.cpp:433-482; R/primitive_pass.cpp:98-216 | changed | a frame-local dedupe exists; the first binding still copies, the cache still compares bytes |
| G3 | F/game_ui_images.cpp:87-113 | present | |
| G7 | F/primitive_constants.cpp:55-111; game_ui_shaders.cpp:107, 130; game_ui_session.cpp:442 | present | |
| G8 | native/content/texture_tiling.cpp:96, 112, 122; F/primitive_textures.cpp:99; primitive_constants.cpp:61 | present | `PrimitiveTextureStore::Withdraw` gone; `PrimitiveUniformSource::Write` and `DecodeCpuTextureContainer` used by tests only |
| G9 | F/game_ui_image_owner.cpp:406-408; game_ui_session.cpp:136-143 | present | |
| G10 | GB/16_offscreen_and_movies.inc:961-971 | present | |
| H2 | GB/04_updates_and_test_tools.inc:594, 722, 739, called at 911 | present | |
| H3 | GB/04_updates_and_test_tools.inc:420-453 | present | |
| H4 | GB/04c_scene_witnesses.inc:585-588 | present | |
| H5 | GB/04b_pass_witnesses.inc:1453, 1513-1518 | present | |
| H6 | GB/04_updates_and_test_tools.inc:965-973 | present | behind pose-observe.flag |
| S2 | GB/12_scene_collect.inc:1209-1210, 1632 | present | |
| S3 | R/shaders/rt_material.hlsli:108-113, 216 | present | `RT_RECORD_UNSCROLLED` exists since for the forward-opacity and map-blend slices |
| S4 | R/rt_scene.cpp:565; rt_material.hlsli:293-298 | present | |
| S5 | GB/12_scene_collect.inc:1240, 1921, 1788 | present | |
| S7 | R/rt_scene.cpp:637-642 | present | |
| S8, S9 | R/shaders/rt_view.hlsl:93-108 | present | `rt-view.flag` only |
| S10 | GB/12_scene_collect.inc:1933-1938 | present | |
| S11 | GB/12_scene_collect.inc:986, 1199-1201, 1897-1899 | present | |
| S12 | R/shaders/rt_view.hlsl:32-34; rt_scene.h:481, 497 | present | |
| S13 | R/rt_scene.cpp:474; GB/12_scene_collect.inc:1837 | present | |
