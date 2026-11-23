#pragma once
#include "model_draw.h"
#include "scene_view.h"
#include "velocity.h"
#include "glow_stage.h"
#include "post_tone_stage.h"
#include "auto_exposure.h"
#include "taa_stage.h"
#include "glare_stage.h"
#include "mask_stage.h"
#include "effect_stage.h"
#include "rt_scene.h"
#include <optional>

namespace ds3native {
// Native deferred lighting of the G-buffer (I-0066 1049, K-0407 order): the
// genuine DS3 programmes loaded by the engine (GXLightAcc VS; DeferredAO, sun,
// CmbAlbedo, Composite PS), the point and spot batches, the light grid, SSS,
// the shadow mask and the shadowed sun below, with constant blocks from the
// engine's own CPU builders. One set a session (I-0137 C4: AcquireNativeLightingPrograms).
struct NativeLightingPrograms {
    std::vector<uint8_t> vertex,ambient,sun,albedo,composite;
    // 1054 point lights: GXLightAcc_DPointLight1..12 then LegacyDPointLight1..12
    // (an empty entry: that batch size is not loaded, its batches are skipped).
    std::array<std::vector<uint8_t>,24> point;
    // I-0106: GXLightAcc_DSpotLight1..4 (1A6690's table by group size; an empty entry: that group size is skipped).
    std::array<std::vector<uint8_t>,4> spot;
    // I-0107: GXLightOrganize (1A36B0's programme at the default tile size 64; empty: no light grid).
    std::vector<uint8_t> organize;
    // 1137 skin SSS stage (140114640): GXSss VS, GXSssBlurX/Y PS (all empty: stage skipped).
    std::vector<uint8_t> sss_vertex,sss_blur_x,sss_blur_y;
    // 1147 env-map regions (pass 4, 675B0): GXEnvMapRegion VS, PS and its parallax-corrected
    // variant (manager +1F0/+1F8; the diffuse variants stay unused while +2B9/+2BB are clear).
    std::vector<uint8_t> env_vertex,env_pixel,env_pixel_pcc;
    // The region box (1400697F0): 8 float3 positions (.rdata 143D20520, 96 bytes) then
    // 14 uint16 strip indices (143D20500, 28 bytes).
    std::vector<uint8_t> env_box;
    // D-0180 (SSAO-NATIVE 1 row 10): GXRend_CompositeShadowMask (empty: t32/t33 stay the neutral white).
    std::vector<uint8_t> mask_composite;
    // D-0180 C4: the mask layer (GXScreenSpaceShadow VS, GXScreenSpaceCascadedShadow_Soft, GXScreenSpaceShadow_Blur, gxposteffect
    // OutputColor) and the shadowed sun GXLightAcc_DDirLightShadowed1 (all or none: the sun stays unshadowed without them).
    std::vector<uint8_t> sun_shadowed;
};
// One selected env-map region (67070 selection, 675B0 draw): CB_ENVMAP_REGION (400 bytes),
// its IBL cube (t30) and the executor's variant choices.
struct NativeEnvRegionDraw {
    std::array<uint8_t,400> constants{};
    bool parallax_corrected{},inside{};
    NativeModelSrvBinding cube;
};
// One 1A1DA0 tile batch: up to 12 point lights of one class, or up to 4 spot lights (1A6690), over a 128x128 tile.
struct NativePointLightBatch {
    uint32_t programme{};            // index into NativeLightingPrograms::point (spot: ::spot)
    bool spot{};                     // I-0106: a 1A6690 group of up to 4 spot lights (128 bytes each)
    int32_t left{},top{},right{},bottom{};
    float depth{};                   // cbLightAccVtxParam.x
    std::vector<uint8_t> lights;     // cbLightAccLightParam: 64 bytes per light
};
struct NativeLightingInput {
    std::shared_ptr<const NativeLightingPrograms> programs;
    // cbLightAccParam (320), cbLight FC_DirLight[2] (576), cbFog FC_FogParamArray[2] (192).
    std::vector<uint8_t> light_acc,light,fog;
    // I-0141: a byte a set of `light` (288 bytes each): 1 where the set is a cutscene's own (its lights 1 to 3 kept on a traced frame,
    // light_producers.h CutsceneLightSet); empty: none.
    std::vector<uint8_t> authored_sets;
    std::vector<NativePointLightBatch> point_batches;
    bool local_lights{};   // I-0132 R11b: the view has point or spot lights (the frame's judgement, cause 8; its batches only where drawn)
    // 1137 SSS stage: cbSSS records (0x120 per profile, the engines CPU copy [obj+0x38]) and
    // cbSceneParam words [0,48) (FC_DepthComputeParam at 32). Empty: stage skipped.
    std::vector<uint8_t> sss_profiles,sss_scene;
    // 1147 IBL: the selected regions of an untraced (menu) view in list order and its cbSceneParam (960 bytes); empty: SpecularAcc
    // stays black (D-0301).
    std::vector<NativeEnvRegionDraw> env_regions;
    std::vector<uint8_t> env_scene;
    // Image regression (2026-10-16, development: no-sun.flag): the sun pass (GXLightAcc_DDirLight1) skipped, to measure its share.
    bool skip_sun{};
    // I-0107: GXLightOrganize's inputs (1A0080 / 1A36B0) for the main view: the point (64 bytes) and spot (128 bytes) records,
    // cbLightAccParam (320 bytes) and the thread groups (the view's extent over the 64-pixel tile); absent: no grid this frame.
    struct LightOrganize {std::vector<uint8_t> points,spots;std::array<uint8_t,320> constants{};uint32_t groups_x{},groups_y{};};
    std::optional<LightOrganize> organize;
    // I-0122 R4a (on with the rays, the main view): the unshadowed local lights as one world-space list with the batches' tiles
    // (RtLightList); present, the renderer shades them natively (RtLocalLights) and draws no batch (I-0132 R11e: a frame whose pass
    // cannot run is unserved, cause 8; empty for a view without local lights).
    std::shared_ptr<const RtLightList> rt_lights;
    // I-0129 R9a (R9-PLAN §2): the frame's traced probes (the main view's, with the rays): their requests and keys, and the states their
    // answers go to once the frame's work completed (RtProbeStates::Deliver, from the slot's completions).
    std::shared_ptr<const RtProbeRequests> rt_probes;
};
// I-0066 1186 velocity family (I-0068 VELOCITY-IMPLEMENTATION-CHECKLIST): the main view's velocity target (cleared to
// zero velocity, then the object Velo rows) and the native motion-vector resolve (upscaler preparation).
struct NativeVelocityInput {
    Matrix4 current_to_previous{};   // (u, v, depth, 1) row -> previous homogeneous uv (13BFC0's g_mCurToPrevScreen)
    std::array<float,4> scale{};     // the Velo rows' FC_DifColMul (their encoding of the NDC delta)
    bool reset{};                    // previous view = current (cut, load, first frame)
    Matrix4 clip_to_previous_clip{}; // I-0111: unjittered current clip -> previous clip (the upscaler's constants)
    Matrix4 still_to_previous{};     // I-0140 tranche 40: a menu view presented again: current_to_previous with the camera as its own previous
    // I-0136 (the Velo rows' depth precision): the rows project absolute world positions where the G-buffer projects
    // camera-relative ones, so against the scene depth their depth is off by rounding that grows with the coordinates' size. With
    // own_depth they are drawn into a depth of their own (the nearest row wins), then a pass keeps a row's pixel unless the scene
    // depth is nearer than it by more than that rounding: occlusion = {|camera position|, near plane, far / (far - near), 0}.
    bool own_depth{true};   // the producer sets the rows' depth state by the same rule (13b); a legacy row would only draw into the rows' depth
    std::array<float,4> occlusion{};
};
// W5d GL-3 (GLOW-WAVE1-READY 5.1): the main view's glow inputs of the frame, present when the blended Glow Filter record has
// Enable and New Glow Enable set, the factor [[GXRS+0x5368]+0x80] is positive and no-glow.flag is absent. The scales are the
// record's (BlurSampleScale for cbSampleOffset), the composites' cbGlowParam.x already faded ((0 - g) s + g with
// s = clamp(1 - factor)), AlphaScale's raw dword for cbGlowParam.y. `programs`: the four pinned blobs (W6 GL-4,
// AcquireNativeGlowPrograms); without them the frame has no glow input.
struct NativeGlowPrograms {GlowBlobs blobs;};
struct NativeGlowInput {
    float blur_scale{1.0f},rgb_scale{},final_scale{};uint32_t alpha_bits{};
    std::shared_ptr<const NativeGlowPrograms> programs;
};
// W10 TM-2 (D-0175): the main view's tone-map pass (on by default, with the YEBIS feed), from the blended Tone Map record.
// Present: the renderer draws it from the lit HDR in place of the sRGB composition. I-0138 tranche 1: our own pass (PostToneStage)
// with its parameter block `post` and the dither texels (tranche 8: YEBIS's programmes, their `$Globals` block and the two test
// paths that drew them are gone; the automatic exposure reads `post`, tranche 3).
struct NativeTonemapInput {std::array<uint8_t,64> dither{};
    PostToneParams post;
    // I-0138 tranche 6: `post` may carry a candidate look (world/post_tone.h ApplyToneLook); post_record is the record's own block
    // (the same as `post` without a look): the automatic exposure reads it, so every look of a view is drawn at one exposure, and a
    // frame whose look's pipeline cannot be made is drawn with it. look_neutral: the look comes without the area's grade (no LUT
    // for this frame, the matrix the identity).
    PostToneParams post_record;bool look_neutral{};
    float exposure{};   // I-0111 tranche 2 (D-0242): the blended Tone Map record's Exposure (word 0), DLSS's exposure on a DLSS frame
    bool vignette{};std::array<std::array<float,2>,3> vignette_uv{};   // TM-3: the vignette variant and its TEXCOORD2
    // I-0132 R11h: the automatic exposure (AutoExposureStage) before the glare and the tone map, its factor the tone map's t2, and
    // its parameters (the game's one target).
    bool auto_exposure{};AutoExposureParams auto_params{};};
// W10 GL-3 (D-0175): the main view's glare family (on by default, with the tone-map input): the native plan (yebis::BuildGlare
// from the library's globals) and the programmes of its techniques (our own). Present: the renderer draws it from the lit HDR before
// the tone map, which takes its output as t1.
// LS-1/LS-3: the plan may carry Alpha_R (pre) and the light shafts; film_grain, the executable's film grain with its CPU mips.
// I-0138 tranche 2: programs are our own (post_glare_programs.h).
struct NativeGlareInput {yebis::GlarePlan plan;std::shared_ptr<const GlarePrograms> programs;std::shared_ptr<const std::vector<std::vector<uint8_t>>> film_grain;};
// W10 DF-4 (W10-NATIVE-PASSES 9): the main view's depth of field (on by default, with the glare): the native plan (yebis::BuildDof from the
// native holder), its programmes (our own) and the DoF table it draws with (the previous frame's, as the original's CustomDofLutCopy
// writes it at the end of a frame for the next). Present: the renderer draws it from the lit HDR before Alpha_R and the glare.
// I-0138 tranche 4: programs as the glare input's (our own).
struct NativeDofInput {yebis::GlarePlan plan;std::shared_ptr<const GlarePrograms> programs;std::vector<float> table;};
// W10 MB-4 (W10-NATIVE-PASSES 10): the main view's motion blur (on by default, with the glare and the velocity input): the native
// plan (yebis::BuildMotionBlur: the camera velocity into the internal velocity, then the blur passes) and its programmes (our own). Present: the
// renderer reduces the velocity target to the engine's half-size RGBA8 object velocity and draws the plan after Alpha_R, copying its output
// back into the lit HDR before the glare (the original's order: depth of field, Alpha_R, motion blur, glare).
// I-0138 tranche 4: programs as the depth of field's.
struct NativeMotionBlurInput {yebis::GlarePlan plan;std::shared_ptr<const GlarePrograms> programs;uint32_t object_width{},object_height{};};
// W10 GR (W10-NATIVE-PASSES 12): the main view's colour grading (on by default, with the tone-map input), present when the blended
// ColorGrading record is enabled and the LUT texture it names (mAA_BB_CGrading<id>, an admitted TPF texture) exists: that texture
// (kept alive by lut_owner). Present: our tone-map pass copies the strip into its volume (its own compute, I-0138 tranche 8) and
// draws the tone map into its intermediate, then through the LUT into the frame's target (the original's order, I-0138 tranche 1).
struct NativeGradingInput {ID3D12Resource* lut{};DXGI_FORMAT lut_format{DXGI_FORMAT_UNKNOWN};uint64_t lut_generation{};std::shared_ptr<const void> lut_owner;};
// I-0126 R7c (D-0251): the main view's shadow mask inputs of the frame (the SSAO that shared them is removed): the two
// cbCompositeShadowMask pairs (10CDD9 for stencil class 0, 10D01E for class 0x10) and `ready` when the pairs were read (I-0131
// R10a: the raster sun layer's rotation image gone with it).
struct NativeShadowMaskInput {
    std::array<std::array<uint8_t,32>,2> composite{};bool ready{};};
// C4-1 (PERSISTENT-RECORDS-DESIGN 2): the canonical frame blocks of one prepared view, by kind, and which kind each frame
// root of a pass binds: the G-buffer draws, the forward draws (PS cbSceneParam with the light-grid words, I-0107) and
// the Velo draws (VS cbMatrixWVP = current and previous view-projection).
enum NativeFrameBlockKind : uint32_t {FrameBlockView,FrameBlockScene,FrameBlockPerFrame,FrameBlockForwardScene,FrameBlockVelocity,FrameBlockKindCount};
enum class NativeModelPass : uint32_t {Deferred,Forward,Velocity};
constexpr std::array<NativeFrameBlockKind,FrameRootCount> NativeFrameBlocksOf(NativeModelPass pass) noexcept {
    return {pass==NativeModelPass::Velocity?FrameBlockVelocity:FrameBlockView,FrameBlockScene,FrameBlockPerFrame,
        pass==NativeModelPass::Forward?FrameBlockForwardScene:FrameBlockScene,FrameBlockPerFrame};}
// Current engine preparation supplies ordered draws retaining the same owned
// inputs/residency. The startup renderer owns targets, commands and completion.
// DECALS-PAINT-READY (D2): one struck character's paint target (GXPaintRenderTarget's diffuse 0x1D and specular 0x1C textures,
// size x size), made by the collect, its RTVs in its own heap; both textures are left PIXEL_SHADER_RESOURCE between frames.
// `cleared` is the render thread's: its first job clears both to (0,0,0,1) (4CD70), the neutral decal.
struct NativePaintTarget {
    Microsoft::WRL::ComPtr<ID3D12Resource> diffuse,specular;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvs;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE,2> rtv{};
    // D-0193 (DECALS-PAINT-READY rung 2b): the projector depth map t32 (D16 paint_occlusion_edge square, left PIXEL_SHADER_RESOURCE),
    // drawn by each job from the struck character's depth programmes, so the paint reaches only the surfaces the projector sees.
    Microsoft::WRL::ComPtr<ID3D12Resource> occlusion;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvs;
    D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
    uint32_t size{};
    mutable bool cleared{};
};
inline constexpr uint32_t paint_occlusion_edge=512;
struct NativePaintJob {
    std::shared_ptr<const NativePaintTarget> target;
    bool clear{};   // a handle-0 command (5F430 type 0) or a type-2 clear of every bit: clear the target again
    bool erase{};   // I-0105 BLOOD: `draws` are 1AAF90's erase pass (pass 10, the _PntCol programmes in second-UV space), no projector depth
    std::array<uint8_t,64> view{};   // VS b0 cbMatrixWVP: the projector matrix F (register j = column j)
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> draws;
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> depth_draws;   // D-0193 rung 2b: the character's Dpt caster rows, into `occlusion`, before `draws`
};
// I-0117 (R2 T1, the rays' scene): the frame's instances for the rays, from the loaded-world walk of its collect; the renderer builds
// the top level from them at the start of the frame's list. The structures they hold live until that frame completed. T2: the
// skinned instances, skinned and built or refitted on that list before the top level (RtSkinner).
// I-0119 R3c (the step review's M1): with the instances, the raster's material samplers' anisotropy (1: not anisotropic) and mip bias
// (the material bank's settings rows 12 and 13: Texture Quality's filter and anisotropy, DLSS's bias), the rays' texture level's.
// I-0125 R6a: `dominant` the frame's cbPerFrame dominant rows the lightmapped ambient reads (ProducePerFrame +0xC0..): FC_DominantRgbScale
// times FC_DominantColorScale in xyz, FC_DominantSharpnesOffset in w; zero when the frame has none.
// I-0125 R6d: `time` the frame's FC_GlobalTime (cbPerFrame +0xB0, the simulation clock the raster's ProducePerFrame takes), the sky's scrolls.
struct NativeRtSceneInput {std::vector<RtInstance> instances;std::vector<RtSkinnedInstance> skinned;uint32_t anisotropy{1};float texture_bias{};
    std::array<float,4> dominant{};float time{};};
// I-0140 tranche 35: the most blur passes a Player menu view's self-shadow takes (the view's plan key holds the count in four bits).
inline constexpr uint32_t kMenuShadowBlurPassesMax=15;
struct NativeModelFrameInput {
    uint64_t serial{};
    // camera.serial identifies the selected CPU view version. Presentation
    // serial is independent; repeated presentations may retain that version.
    SceneViewInput camera;
    uint8_t clear_stencil{};
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> draws;
    // Forward pass (engine pass 0x29, shader pass 0 Fwd): one-target draws into
    // the lit HDR image after deferred lighting, depth tested against the
    // G-buffer depth. Requires lighting.
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> forward_draws;
    // I-0140 (gap 23): the forward draws (held by forward_draws) whose model's fade moved this frame, which draw into the TAA's fade
    // mask (their fade-mask pipelines, model_draw.h fade_mask); empty on most frames.
    std::vector<const OwnedNativeModelDraw*> fade_mask_draws;
    // I-0113 tranche 15: the forward draws in the original's order (0x1F, 0x21, 0x22, 0x29, each by 57A20's key); forward_keys
    // the key of each, forward_sorted the first 0x29 draw, which the encoder interleaves with the 0x29 effects by key.
    std::vector<uint32_t> forward_keys;size_t forward_sorted{};
    // I-0140 tranche 4: with the keys, the end of the 0x1F rows and of the 0x21 rows (forward_1f <= forward_21 <= forward_sorted),
    // where the encoder draws pass 0x21's effects among the 0x21 rows, before the 0x22 rows and the water (subs 0x24, 0x27).
    size_t forward_1f{},forward_21{};
    // Absent: the image shows material colour (G-buffer 2), as before 1049.
    std::shared_ptr<const NativeLightingInput> lighting;
    // 1186: present when the MotionBlur option allows velocity; the Velo rows (engine pass 0x34, one velocity target,
    // depth tested against the scene depth) follow the forward pass.
    std::shared_ptr<const NativeVelocityInput> velocity;
    // I-0111 tranche 4 step 2: DLSS for this frame, decided at its collect (NativeSelectedScene::dlss: the game's Antialiasing option
    // on and Streamline ready at that frame's BeginFrame).
    bool dlss{};
    // I-0132 R11j: the TAA for this frame (NativeSelectedScene::taa: the Antialiasing option on, no DLSS mode running), the tone
    // map's exposure its weights' scale.
    bool taa{};
    // I-0113 tranche 39: the TAA's reactive base (the lit HDR before the 0x29 rows and the effects), unless no-taa-reactive.flag.
    bool taa_reactive{};
    bool taa_reactive_view{};   // I-0113 tranche 41: taa-reactive-view.flag, the reactive weight painted red (a test view)
    float sharpen{};   // Thomas's sharpening after the TAA, 0 to 1 (NativeSharpen)
    float taa_kernel{};   // I-0140 tranche 41 (D-0306): a menu view's TAA reconstruction (TaaParams::kernel; 0: the stage's 2.29); the main view's taa-kernel.txt (I-0138 tranche 9)
    bool taa_reset_always{};   // I-0138 tranche 9: taa-reset-always.flag, a test tool (the main view's TAA without its history)
    std::shared_ptr<const NativeGlowInput> glow;   // W5d GL-3: main view only; no consumer in W5 (the stage is W6)
    std::shared_ptr<const NativeTonemapInput> tonemap;   // W10 TM-2: main view only
    std::shared_ptr<const NativeGlareInput> yebis_glare;  // W10 GL-3: main view only, with the tone-map input
    std::shared_ptr<const NativeDofInput> yebis_dof;      // W10 DF-4: main view only, with the glare
    std::shared_ptr<const NativeMotionBlurInput> yebis_motionblur;   // W10 MB-4: main view only, with the glare and the velocity
    std::shared_ptr<const NativeGradingInput> yebis_grading;   // W10 GR: main view only, with the tone-map input
    std::shared_ptr<const NativeShadowMaskInput> mask;   // I-0126 R7c: main view only (the shadow mask t32's inputs)
    // FFX-DRAW N6: the classic FFX draws of the main view (on by default, no-ffx-draw.flag), drawn after the forward rows into the lit HDR (and GB5
    // for the glow twins), depth tested read-only against the scene depth they also sample.
    std::shared_ptr<const NativeEffectFrame> effects;
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> velocity_draws;
    // DECALS-WAVE1 D1: the deferred box decals (gxdecal kind 0; Decal7 envelope, depth read-only, the stencil receiver test),
    // drawn after the G-buffer and before the lighting as the original's 51AA0, with the copies they sample made first from
    // GB0 (t13), GB1 (t14) and the depth (t4) into the renderer's DecalSources() (main view; requires lighting).
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> decal_draws;
    // DECALS-PAINT-READY rung 2 (D2): the paint jobs of this frame, encoded before the G-buffer (the targets the _Bloody
    // materials sample at t41/t42): a target's first job clears it to (0,0,0,1); a job with draws then paints the struck
    // character's meshes in their second-UV layout with the projector's VS b0 (cbMatrixWVP, `view`), or erases them toward the
    // neutral (an erase job, I-0105).
    std::vector<NativePaintJob> paints;
    // WATER-TESS-WAVE1-READY WT-2..WT-4: the water rows of the main view, WatDpt (engine 0x24) and the surface (0x27, Fwd), drawn
    // after the first `forward_early` forward draws (the 0x1F, 0x21 and 0x22 rows; I-0113 tranche 15: the subs 0x24 and 0x27 come
    // before 0x29 only) with the readable depth copy (t56, the resource their t56 views)
    // made from the depth just before, and the frame copy taken twice (C1 after WatDpt for the surface's t55, C2 after the surface).
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> water_depth_draws,water_draws;
    size_t forward_early{};
    Microsoft::WRL::ComPtr<ID3D12Resource> water_depth_copy;
    Microsoft::WRL::ComPtr<ID3D12Resource> decal_gb0_copy,decal_gb1_copy,decal_depth_copy;
    // C4-1: each kind's block, the longest packed block of that kind among the draws (the others are its prefixes); null
    // when no draw binds the kind. The recording thread copies each once into the slot's upload arena.
    std::array<std::shared_ptr<const std::vector<uint8_t>>,FrameBlockKindCount> frame_blocks;
    uint64_t primitive_generation{};
    // D-0180 C4; I-0131 R10a: the traced sun's shadow blocks (with the shadow frame): the view's cbSceneParam with the shadow words
    // (+0x1C0..+0x36F, 960 bytes; FC_ShadowColor for the shadowed sun pass), the sun and the gate. The cascades, the atlases, the
    // casters, cbPerScene and the blur are removed.
    std::array<uint8_t,960> shadow_scene{};
    std::array<float,4> rt_sun{};   // I-0117 R5: the LightSet's light 0 (toward the sun, world)
    bool shadow_blocks{};           // I-0118 R5a: shadow_scene and rt_sun written this frame (the traced pass's gate)
    // I-0140 tranche 35: a Player menu view's blur of its traced self-shadow, the original's (its ShadowParam's Blur Count plus the
    // quality row's, its Blur Radius times [[GXRS+0x5368]+0x128]): the passes (0: none) and their scale (pixels a tap unit).
    uint32_t self_shadow_blur_passes{};float self_shadow_blur_scale{};
    // I-0125 R6c (R6-PLAN §6): the water's reflection traced this frame (the collect's choice: the rays and the pass on, a water plane
    // found): the target t54 binds (kRtWaterFormat, ALLOW_RENDER_TARGET, shader-readable between frames) and
    // the water's plane (n·X + w = 0, n the water's up).
    struct RtWater {Microsoft::WRL::ComPtr<ID3D12Resource> target;std::array<float,4> plane{};};
    std::optional<RtWater> rt_water;
    bool initialize_primitive_stencil{};
    uint8_t primitive_initial_stencil{};
    std::shared_ptr<const NativeRtSceneInput> rt_scene;   // I-0117 (R2 T1): main view only
    // I-0132 R11e step 3: an arrival's frame held until the rays' scene is ready: its world black, the UI drawn over it (main view only).
    bool rt_hold{};
};
}
