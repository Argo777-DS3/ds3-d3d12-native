# I-0137 step 2: locations of the `Unsupported*` stubs and I-0101's stops

Sonnet locating pass, condensed; each row re-read in the code by the pilot in tranche 6 (its line numbers held), its classes
replaced by the pilot's in RESULT « Tranche 6 ». Worktree
`claude/sharp-tereshkova-5cbcb6`, 2026-11-12. Paths are under `native/`; line numbers are as of this tree.
Terms: Refuse = `Refuse(...)` of game_bootstrap (module exit with a dump); Fail = the owner's `Fail(...)` then `std::terminate`.
"Table size" = size of the table the stub fills before the real methods overwrite some entries; the number of slots left
as stubs is the table size minus the overwritten ones (not counted here).

## 1. The `Unsupported*` stubs and where they are installed

| # | Stub (file:line) | Installed at | Table / object | Table size | What it does |
|---|---|---|---|---|---|
| 1 | `UnsupportedSceneContext` frame/game_bootstrap/03_engine_cpu_owners.inc:465 | :487 (`NativeSceneContextTable`, `result.fill`; slot 0 = `RetireNativeSceneContext`) | vtable of the native scene context | 3 | Refuse `native_scene_context_method_not_owned` |
| 2 | `UnsupportedSceneView` 03_engine_cpu_owners.inc:539 | :565 (`NativeSceneViewTable`, fill; slots 0-4 and others set) | vtable of the native scene view | 14 | Refuse `native_scene_view_submission_not_owned` |
| 3 | `UnsupportedModelStorage` game_bootstrap/14_scene_step_and_storage.inc:195 | :221 (`ModelStorageTable`, fill; slot 0 = Destroy) | vtable of model storage objects | 64 | logs `native_model_method_unowned` (caller rva), then Refuse `native_model_storage_method_not_owned` |
| 4 | same stub | 18_resource_admission.inc:404 (`ModelMaterialTable`, fill) | vtable of material instances (slot 0 = Destroy; slot 6 noted unhandled, I-0101 L7) | 64 | same as 3 |
| 5 | same stub | 18_resource_admission.inc:648 (table built for the native model resolver, `ConstructNativeModelResolver` region, fill; slot 0x20/8 = BuildModelLayout) | vtable of the native model resolver | 64 | same as 3 |
| 6 | same stub | 18_resource_admission.inc:362 (`result[1]`; slot 0 = DestroyModelLayout) | vtable of a model layout object | 2 (1 stub slot) | same as 3 |
| 7 | `Unsupported` frame/game_ui_cpu_shape_context.cpp:32 | :41 (`state.table.fill`; slot 0xF0/8 = Scratch) | CPU shape context vtable | 31 | `Owner(object).Fail("unqualified_native_cpu_shape_context_method")`, terminate |
| 8 | `UnsupportedManager` frame/game_ui_image_owner.cpp:78 | :369 (`manager_table.fill`, `BuildTables`) | UI image manager vtable | 29 | Fail `unqualified_native_ui_manager_method`, terminate |
| 9 | `UnsupportedImage` game_ui_image_owner.cpp:79 | :370 (`image_table.fill`) | UI image vtable | 24 | Fail `unqualified_native_ui_image_method`, terminate |
| 10 | `UnsupportedFormat` game_ui_image_owner.cpp:84 | :388 (`format_table` slot 0, the retirement slot) | borrowed format object vtable | 4 (1 stub slot) | Fail `native_ui_borrowed_format_retirement`, terminate |
| 11 | `Unsupported<Slot>` (template) frame/game_ui_mesh_owner.cpp:99 | :101-102 `DefaultMethods`, called :312 with `make_index_sequence<19>` | mesh cache vtable `state.table` | 19 | Fail `unqualified_native_mesh_cache_method_%03zX`, terminate |
| 12 | `UnsupportedSecondary` game_ui_mesh_owner.cpp:103 | :313 (`state_->secondary` slot 1) | mesh cache secondary vtable | 3 (1 stub slot) | Fail `unqualified_native_mesh_cache_secondary`, terminate |
| 13 | `Unsupported<Slot>` (template) frame/game_ui_session.cpp:168 | :171-172 `DefaultMethods`, called :1080 with `make_index_sequence<128>` | Scaleform HAL vtable `state.table` | 128 | Fail `native_ui_session_method_%03zX`, terminate (the HAL's never-qualified methods, I-0137 RESULT) |
| 14 | `GameUiCpuShaderPolicy::Unsupported` frame/game_ui_shaders.cpp:69 (decl .h:38) | :74 (`table_.fill`) | CPU shader policy vtable | 14 | `failure("unqualified_native_cpu_shader_policy")`, terminate |
| 15 | `FilterObjectUnsupported` frame/game_ui_session.cpp:710 | :719 `filter_target_table`, :721 `filter_texture_table` (fill; slots 0x18/8 and 0x68/8 set) | Scaleform filter temporary target and its texture vtables | 16 + 16 | inside a filter scope: Fail `native_ui_filter_object_method`; outside it: `std::terminate` |

Not stubs (name match only: enum values or status helpers, no refusal by themselves): `ModelPaletteIndexStatus::Unsupported`,
`ModelGeneratedStatus::Unsupported`, `NativeModelPixelStatus/PassStatus`, `ModelProgramStatus`, `ModelTextureStatus::Unsupported`,
`ModelPoseStatus::UnsupportedNongated/Fast`, `ShaderConstantStatus::Unsupported`, `PrimitiveConstantStatus/TextureStatus/WorkStatus::Unsupported`,
`PrimitiveProgramStatus::UnsupportedMode`, `CpuUiMeshOutputStatus::Unsupported`, `UnsupportedModelShaderTexture` (content/model_textures.cpp:68, a query),
`primitive_work.cpp:11 struct Unsupported` (an internal exception, caught at :203). Their consumers may refuse; not listed here.

Totals: 15 stub rows; 12 distinct stub functions/templates (SceneContext, SceneView, ModelStorage, cpu-shape `Unsupported`,
Manager, Image, Format, mesh `Unsupported<Slot>`, mesh Secondary, session `Unsupported<Slot>`, shader policy, FilterObject);
`UnsupportedModelStorage` is installed in 4 tables.

## 2. I-0101's stops

Source note: `investigations/I-0101-evening-faults/STUDY.md` lists only three refusals (present, pose, model method). The
unsorted stops by family are in I-0101 `RESULT.md` section « The triage of the module's refusals » (lines 47-70; closing at ~141:
the asset loaders' refusals, the movie outputs, the catch-alls, the cloth sets, the walk's runaway bounds, the gradient
filters (M2), L4 and L7) and I-0137 `RESULT.md` ~360-368. Census there: 488 `Refuse(` sites, 366 own contract, 58 one-object
data with a natural skip, 64 unclear. The families below are found by grep; the class is MY first reading (A = game data,
candidate to skip and count; B = own contract, keep; C = mixed or the consumer must be read first).

| Family | Stop (string) | file:line today | Present | Class | Note |
|---|---|---|---|---|---|
| Walk's bounds | `native_scene_selection_object_bound` (32,768 objects) | game_bootstrap/09_model_admission.inc:1174 | yes | A/C | runaway bound on game data; a skip must stop the walk cleanly |
| Walk's bounds | `native_scene_selection_disp_children_bound` (4,096 children) | 09_model_admission.inc:1246 | yes | A/C | the RT walk already skips (`rt_walk_disp_children_bound`, I-0117); the raster walk still refuses |
| Walk's bounds | `native_scene_tree_cpu_arena_bound` (64 MB) | 03_engine_cpu_owners.inc:403 | yes | B/C | the module's own arena; keep unless driven by data |
| Walk's bounds | `native_scene_tree_cpu_bounds_source` | 03_engine_cpu_owners.inc:425 | yes | C | read the site |
| Movies | `native_movie_bink_import` | 16_offscreen_and_movies.inc:639 | yes | B | the module's check of the binkw64.dll import |
| Movies | `native_movie_output_unknown` | 16_offscreen_and_movies.inc:779, :856, :893 | yes (3 sites) | C | an engine handle the module does not own: contract or data |
| Movies | `native_movie_plane_set` | 16_offscreen_and_movies.inc:852 | yes | A | plane set index from the engine |
| Movies | `native_movie_plane_shape` | 16_offscreen_and_movies.inc:861 | yes | A | video plane geometry |
| Movies | `native_movie_plane_buffer` | 16_offscreen_and_movies.inc:865 | yes | A | buffer/pitch from the decoder |
| Movies | `native_movie_target_failed` | 16_offscreen_and_movies.inc:932 | yes | C | catch of std::exception (allocation or GPU) |
| Movies | `native_movie_frame_failed` | 16_offscreen_and_movies.inc:977 | yes | C | catch-all |
| Movies (UI) | `native_ui_movie_viewport_contract` | 17_render_step.inc:446 | yes | C | |
| Movies (UI) | `native_ui_movie_frame_count`, `native_ui_movie_frame_null` | frame/game_ui_session.cpp:1180, :1183 (Fail) | yes | C | |
| Cloth | `native_cloth_vertex_upload_failed` | 17_render_step.inc:194 | yes | C | catch(...) around the upload |
| Cloth | `native_cloth_set_release_failed` | 17_render_step.inc:206 | yes | C | catch(...) around the release (bounded read of 4096 entries) |
| Cloth | `native_cloth_set_failed` | 17_render_step.inc:279 | yes | C | catch(const std::exception&); row `native_cloth_set_failure` logged first |
| Asset loaders (material) | `material_cpu_string_extent/_pointer/_unreadable`, `material_original_gpu_association`, `loaded_material_cpu_identity_mismatch`, `material_cpu_bytes_unreadable`, `material_cpu_admission_failed` | 18_resource_admission.inc:150, 151, 153, 159, 228, 231, 250 | yes | A/C | the loader's handle is consumed by the original: a skip must not become a refusal further on (I-0101) |
| Asset loaders (material params) | `native_material_parameter_*` (parse_failed, count, node_missing, name_extent, name_missing, name_unreadable, name_bound, name_collision, value_extent, values_missing, values_invalid) | 18_resource_admission.inc:179-214 (name_bound :198) | yes | A | parsing of the game's material data |
| Asset loaders (effect) | `effect_cpu_*` (archive_unavailable, allocation_failed, archive_decode_failed, byte_count_unreadable, reader_unavailable, archive_entry_bound :90, declaration_extent, declarations_unreadable, program_table, program_record, program_name, program_name_unreadable, temporary_heap_unavailable, admission_failed), `effect_native_program_missing`, `unexpected_original_effect_pipeline` | 18_resource_admission.inc:50-144 | yes | A/C | allocation and heap ones are B |
| Asset loaders (shader) | `shader_input_extent/_unreadable/_not_dxbc`, `shader_container_extent`, `shader_reflection_failed`, `shader_model_unsupported`, `shader_wrapper_has_legacy_object`, `unexpected_legacy_shader_handle`, `native_shader_admission_failed` | 18_resource_admission.inc:11-46 | yes | A/C | the first six read the game's DXBC |
| Asset loaders (gparam) | `gparam_cpu_bytes_unreadable`, `gparam_cpu_kind_extent`, `gparam_original_gpu_handle`, `gparam_cpu_fallback_heap/_allocation` | 18_resource_admission.inc:687-727 | yes | A/C | |
| Asset loaders (texture) | `texture_asset_family_not_owned` :483, `unexpected_original_texture_handle` :464, `native_texture_admission_failed` :497, `native_encoded_texture_admission_failed` :643, `direct_dds_cpu_allocator_unreadable` :634, `texture_cpu_allocator_unavailable` :488 | game_bootstrap/14_scene_step_and_storage.inc | yes | A/C | also `legacy_encoded_texture_source` 02_frame_lifecycle.inc:149; stage textures 03_engine_cpu_owners.inc:634-678 |
| Asset loaders (UI) | `native_ui_loader_heap_unavailable` | frame/game_ui_session.cpp:1232 (Fail) | yes | B | heap of the engine loader at teardown |
| Catch-alls | the `catch(...)` Refuse `*_failed` family, e.g. `native_scene_tree_cpu_retirement_failed` 03_engine_cpu_owners.inc:464, `native_model_storage_retirement_failed` 14_scene_step_and_storage.inc:217 | many | yes | C | mixes data, allocation and GPU failures |
| Degraded filters | gradient kinds 4 and 5: `native_ui_filter_gradient_unavailable` | frame/game_ui_session.cpp:838 (Fail) | yes | A | I-0101 M2; `NativeFiltersSupported` refuses kinds above 2, so the branch is unreachable |
| Degraded filters | pass without programmes or layout: `native_ui_filter_vertex_layout` :854, `native_ui_filter_source_unbound` :855, `native_ui_filter_sampler` :865/:867, `native_ui_filter_plan_changed` :982 | game_ui_session.cpp | yes | A/C | I-0137 M4: a skipped middle pass blanks the chain; the fallback is the target refusal's (pop and draw unfiltered) |
| Degraded filters | scope and ownership: `_push_scope` :938, `_pop_scope` :1037, `_scope_mismatch` :1041, `_target_not_owned` :1039, `_target_depth` :746, `_target_underflow` :783, `_capability_scope` :912, `_capability_extent` :921 (all `native_ui_filter_*`), `native_ui_cached_filter_target_not_owned` :193, `native_ui_frame_filter_scope` :1169, `native_ui_filter_texture_bind` :716, `native_ui_filter_object_method` :711 | game_ui_session.cpp | yes | B | the module's own scopes and objects |
| Other (I-0101 L4) | non-device failure on a mode change | not located by string | ? | C | the `TryResizeBuffers` path, see I-0101 RESULT L4 |
| Other (I-0101 L7) | per-object refusals of the `05_selected_scene.inc` group and policy unreadable; slot 6 of the material table | 05_selected_scene.inc (not located); material table 18_resource_admission.inc:402 | slot-6 stub yes (row 4 of table 1) | A | DS3's FUN_140EF1F80 not handled |
| Other (aperture) | `an aperture filter other than the tap blurs` | world/yebis_dof.cpp:158 (`refuse(`) | yes | C | not a game_bootstrap Refuse; a degraded-filter case of the depth of field |

Removed since the study (not present today as refusals): `native_model_pose_nonfinite_current_rows` (05_selected_scene.inc; a skip row
`native_model_pose_nonfinite` is at :101) and the Present-failure refusal (now `native_present_failed` handling), converted in I-0101 tranche 1.

## 3. `Refuse("...")` in `native/frame/game_bootstrap/*.inc` whose string contains loader, movie, bink, cloth, walk, bound or filter

Case-insensitive grep on the string. No string contains `loader`, `walk` or `filter`. 19 hits.

| # | file:line | String |
|---|---|---|
| 1 | 03_engine_cpu_owners.inc:403 | `native_scene_tree_cpu_arena_bound` |
| 2 | 03_engine_cpu_owners.inc:425 | `native_scene_tree_cpu_bounds_source` |
| 3 | 09_model_admission.inc:1174 | `native_scene_selection_object_bound` |
| 4 | 09_model_admission.inc:1246 | `native_scene_selection_disp_children_bound` |
| 5 | 16_offscreen_and_movies.inc:639 | `native_movie_bink_import` |
| 6 | 16_offscreen_and_movies.inc:779 | `native_movie_output_unknown` |
| 7 | 16_offscreen_and_movies.inc:852 | `native_movie_plane_set` |
| 8 | 16_offscreen_and_movies.inc:856 | `native_movie_output_unknown` |
| 9 | 16_offscreen_and_movies.inc:861 | `native_movie_plane_shape` |
| 10 | 16_offscreen_and_movies.inc:865 | `native_movie_plane_buffer` |
| 11 | 16_offscreen_and_movies.inc:893 | `native_movie_output_unknown` |
| 12 | 16_offscreen_and_movies.inc:932 | `native_movie_target_failed` |
| 13 | 16_offscreen_and_movies.inc:977 | `native_movie_frame_failed` |
| 14 | 17_render_step.inc:194 | `native_cloth_vertex_upload_failed` |
| 15 | 17_render_step.inc:206 | `native_cloth_set_release_failed` |
| 16 | 17_render_step.inc:279 | `native_cloth_set_failed` |
| 17 | 17_render_step.inc:446 | `native_ui_movie_viewport_contract` |
| 18 | 18_resource_admission.inc:90 | `effect_cpu_archive_entry_bound` |
| 19 | 18_resource_admission.inc:198 | `native_material_parameter_name_bound` |

Outside game_bootstrap (Fail, not Refuse; same words): game_ui_session.cpp holds 17 `native_ui_filter_*` Fail sites (table 2),
`native_ui_movie_frame_count/_null` :1180/:1183, `native_ui_loader_heap_unavailable` :1232; game_ui_mesh_owner.cpp:204
`native_cpu_mesh_writer_foreign_bound_batch`.
