// PrepareNativeModelFrame: the frame's model draws prepared from the selected scene. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part
// (13_model_frame_inputs.inc before it).
#pragma once
#include "13_model_frame_inputs.inc"
namespace {
// I-0107 (1400764D0): the forward rows' light-grid words of cbSceneParam: +884 the slice scale 0.8, +888 the bias -2.1 (the
// constants of [cam8+0x4B8/+0x4BC]), +896 the tile size 64 and +900 the tiles across, ceil(W / 64) (integers, the bytes of
// [cam8+0x4CC] and the division), when the main view's grid is bound to its forward rows (the record epochs carry it); zero
// otherwise, as before (the rows then read the empty light buffers). +880 FC_FrustumNear stays the camera block's.
template<class Bytes,class Written> void NativeForwardGridWords(const NativeSelectedScene& scene,bool offscreen,Bytes& bytes,Written& written) {
    const bool grid=!offscreen && scene.record_epochs && scene.record_epochs->light_grid && scene.camera.width;
    const float scale=grid?0.8f:0.f,bias=grid?-2.1f:0.f;const uint32_t tile=grid?64u:0u,across=grid?(scene.camera.width+63)/64:0u;
    std::memcpy(bytes.data()+884,&scale,4);std::memcpy(bytes.data()+888,&bias,4);std::memcpy(bytes.data()+896,&tile,4);std::memcpy(bytes.data()+900,&across,4);
    for(const uint32_t at:{884u,888u,896u,900u})written[at/4]=1;
}
// I-0113 tranche 15 (57A20, 5C0E0, BF330 and BF210; PASS-CHUNKS-STUDY « the key »): the forward rows in the original's order. Each
// sub-pass's rows are recorded with a key and drawn sorted (0x1F and 0x29 reversed: far to near; 0x21 and 0x22 near to far), the
// sub-passes in their order 0x1F, 0x21, 0x22, 0x29 (the scene view's key: the sub in bits 16 to 21, the whole sorted ascending). A row's key: an E03 particle its command's (the pass context recorded it); a
// model the emitter's box and bias (NativeSelectedModel::sort, world/sort_key.h) at the frame's camera, 57A20 moving the bias for
// a draw with its descriptor's byte +0x10: at 0x29 the depth pre-pass +0.5 and the colour rows -0.5 (57A20:550-569, 657-720), at
// 0x22 +0.5 (57A20:640-660; world/sort_key.h RowBiasShift). Equal keys keep the walk's order (the original's merge sort 14012A4B0 is
// stable, PASS-CHUNKS-STUDY point 3; the order its recorder threads queue their tables in is not known). A model without a box (a
// part whose owner is not a character) takes its placement's translation with no bias (explicit policy, counted). Every view's rows
// (the menu views too, as 57A20 records every view's), each with its camera; a camera without a forward axis
// keys every model row 0 (explicit policy: the rows then keep the walk's order). Returns the rows without a box.
size_t SortNativeForwardRows(const NativeSelectedScene& scene,NativeModelFrameInput& frame,const std::vector<size_t>& rows) {
    namespace key=ds3native::sort_key;
    float row[4]{};const float far_plane=scene.camera.far_plane;
    const bool view=key::DepthRow(scene.camera.camera_pose.data(),row) && far_plane>0.0f;
    std::vector<uint64_t> order(rows.size());size_t unboxed=0;
    for(size_t i=0;i<rows.size();++i){const auto& p=scene.prepared_meshes[rows[i]];const auto& m=scene.models[p.model];uint32_t k=0;
        if(m.effect)k=m.effect->submission.key&0xFFFF;
        else if(view){float centre[3]{},bias=0.0f;
            if(m.sort){const auto& b=m.sort->box;key::Centre(b.data(),centre);
                bias=m.sort->rule==2?key::HalfDiagonalBias(b.data(),m.sort->extra):key::ViewExtentBias(row,b.data(),m.sort->extra);}
            else{centre[0]=m.placement[3];centre[1]=m.placement[7];centre[2]=m.placement[11];++unboxed;}
            bias+=key::RowBiasShift(p.engine_pass,m.fade_prepass,m.ordinary_category && (*m.ordinary_category==2 || *m.ordinary_category==0x29),
                p.recipe==NativeRecipe(2,0) || p.recipe==NativeRecipe(2,1));
            k=key::PassKey(p.engine_pass,row,far_plane,centre,bias);}
        order[i]=key::OrderWord(p.engine_pass,k,static_cast<uint32_t>(i));}
    std::sort(order.begin(),order.end());   // the index in the low bits: stable
    std::vector<std::shared_ptr<const OwnedNativeModelDraw>> draws(order.size());frame.forward_keys.resize(order.size());frame.forward_sorted=order.size();
    frame.forward_1f=0;frame.forward_21=0;   // I-0140 tranche 4: the ends of the 0x1F and 0x21 rows
    for(size_t i=0;i<order.size();++i){draws[i]=std::move(frame.forward_draws[order[i]&0xFFFFFFFFu]);frame.forward_keys[i]=static_cast<uint32_t>(order[i]>>32&0xFFFF);
        frame.forward_1f+=order[i]>>48==key::ForwardRank(0x1F);frame.forward_21+=order[i]>>48<=key::ForwardRank(0x21);
        if(frame.forward_sorted==order.size() && order[i]>>48==key::ForwardRank(0x29))frame.forward_sorted=i;}
    frame.forward_draws=std::move(draws);
    // The water rows' subs 0x24 and 0x27 come between 0x22 and 0x29 in the key's order (PASS-CHUNKS-STUDY: the sub is the key's bits
    // 16 to 21, sorted ascending): the rows before the water are the 0x1F, 0x21 and 0x22 rows (before it the 0x1F rows alone).
    frame.forward_early=frame.forward_sorted;
    return unboxed;
}
// I-0132 R11h (R11-PLAN section 9 step 1; on by default, D-0264): the automatic exposure's target, one for the whole game (D-0263:
// one universal rule, no target by area): the mean displayed grey the original shows, the trimmed mean of the luma of its pictures at
// the witness views averaged, 0.20 (R11h's first slot, 2026-11-09: 0.09 to 0.37 by view; Measure-AutoExposureTarget.py; RESULT « R11h »).
// D-0294 (Thomas, 2026-11-16; I-0141 RESULT « Tranche 10 »): 0.12, the original's darkest tenth (its tour pictures' tenth percentile 0.116,
// median 0.19): with D-0280's lower bound the exposure brightens only the views darker than that, the others at the zone's own.
inline constexpr float kNativeAutoExposureTarget=0.12f;
float NativeAutoExposureTarget() noexcept {return kNativeAutoExposureTarget;}
// I-0140 (gap 23, the step review of its final review's fixes, H1): whether a model's fade value (its group's +0xDC, mulCol.w) moved
// since the last prepare that read it: a dissolve in progress (a fast travel's 0.997 to 0.017, a death, a spell's fade in or out), not
// a translucency that lasts (Hidden Body held near 0.4 after its descent, I-0113 tranche 10d's reference rows). A kept row is not asked:
// its model's inputs, the group bytes among them, are unchanged. One entry per (entity, model) (the prepare's chunks share it, under a
// lock), the first row of a frame deciding for the others. A model seen for the first time is not moving (the first frame of a
// dissolve is not marked: its value then about 0.997 in a fast travel); an entry unread for 600 serials is dropped once the map holds
// 1,024 (its step review's L2: a model away for a while comes back unknown, not compared with an old value), so the map stays bounded.
bool NativeFadeMoving(uintptr_t entity,uintptr_t model,float value,uint64_t frame_serial) {
    struct Entry {float value{};uint64_t serial{};bool moving{};};
    static std::mutex lock;static std::map<std::pair<uintptr_t,uintptr_t>,Entry> entries;
    const std::lock_guard hold(lock);
    if(entries.size()>=1024)std::erase_if(entries,[&](const auto& entry){return entry.second.serial+600<frame_serial;});
    if(entries.size()>=4096)entries.clear();
    const auto [found,fresh]=entries.try_emplace({entity,model},Entry{value,frame_serial,false});
    if(fresh)return false;
    auto& entry=found->second;if(entry.serial==frame_serial)return entry.moving;
    // A value last read more than 600 serials ago is not compared (the step review's L2).
    entry.moving=entry.serial+600>=frame_serial && !NativeSameBits(entry.value,value);entry.value=value;entry.serial=frame_serial;return entry.moving;}
std::shared_ptr<NativeModelFrameInput> PrepareNativeModelFrame(    const std::shared_ptr<const NativeSelectedScene>& scene,uint64_t presentation_serial,bool offscreen) {
    const kprofile::Scope k_prepare(offscreen?kprofile::None:kprofile::PrepRowSetup);   // cP0: the main view only
    // C4-2 review: a reset (area change, title) drops the kept draws at the next main prepare, even one without rows.
    if(!offscreen && native_kept_draw_resets.load(std::memory_order_acquire)!=native_kept_draws.resets_seen.load(std::memory_order_acquire)){
        const std::lock_guard hold(native_kept_draws.lock);const auto resets=native_kept_draw_resets.load(std::memory_order_acquire);
        native_kept_draws.draws.clear();native_kept_draws.models.clear();native_kept_draws.epoch.reset();native_kept_draws.resets_seen=resets;}
    std::map<int32_t,NativeAsmOutcome> all_asm_outcomes;std::map<std::string,NativeAsmOutcome> all_family_outcomes;
    if(scene && scene->prepared_meshes.empty())LogNativeAsmSummary(*scene,all_asm_outcomes);
    if(!scene || scene->prepared_meshes.empty() || !scene->sampler_bank ||
       (!offscreen && (scene->camera.width!=NativeRenderWidth() || scene->camera.height!=NativeRenderHeight() ||   // I-0111: the render extent,
        (render_width && !(scene->dlss && ds3native::streamline::DlssReady()))))){   // and none while a scaled extent waits to return after DLSS's latch (step review M1)
        if(scene && !offscreen && PopinObserve())PopinNativeFrame(*scene,{});   // I-0100 T0 (step review M3): a main view not drawn: its models off (no main row)
        return {};}
    auto frame=std::make_shared<NativeModelFrameInput>();frame->serial=presentation_serial;frame->camera=scene->camera;
    // DECALS-PAINT-READY rung 2 (main view): one job per paint event; each G-buffer row of a painted model (its patch) whose pixel
    // programme samples the paint (PS t41, INFERRED for 1401407D0's policy bit 2) adds its paint draw once to its event's job. I-0105:
    // an erase event's job takes each of the receiver's G-buffer rows that samples t41 and whose family has an erase programme (1AAF90 draws the
    // receiver's meshes in pass 10; a mesh without a _PntCol programme draws nothing there, INFERRED).
    std::set<std::tuple<size_t,size_t,size_t>> painted_rows;std::array<uint64_t,10> paint_counts{};   // rows, drawn, no set, refused, no t41, depth refused (D-0193), erase rows, erase refused, no erase programme, erase no t41
        // Review: a scene presented again (no newer collect) does not paint, erase or clear again (the blends accumulate; a clear again
        // would wipe the paint drawn after it); a new target's first clear stays (EncodeNativePaint, `cleared`).
    const bool paints_fresh=!offscreen && !scene->paints.empty() && scene->paints_taken && !scene->paints_taken->exchange(true);
    std::vector<size_t> paint_jobs(scene->paints.size(),SIZE_MAX);   // each event's job in frame->paints
    if(!offscreen)for(size_t e=0;e<scene->paints.size();++e){const auto& event=scene->paints[e];if((event.draw || event.erase || event.clear) && !paints_fresh)continue;
        paint_jobs[e]=frame->paints.size();auto& job=frame->paints.emplace_back();job.target=event.target;job.clear=event.clear;job.erase=event.erase;job.view=event.view;}
    const auto apply_paint_row=[&](const NativeSelectedModel& model,const NativeSelectedMesh& mesh,const NativeSelectedScene::PreparedMesh& prepared,
        const std::shared_ptr<const OwnedNativeModelDraw>& draw){
        if(offscreen || frame->paints.empty() || !model.patch || !draw || prepared.recipe>5)return;
        for(size_t e=0;e<scene->paints.size();++e){const auto& event=scene->paints[e];if((!event.draw && !event.erase) || event.patch!=model.patch)continue;
            if(paint_jobs[e]==SIZE_MAX || !painted_rows.emplace(e,prepared.model,prepared.mesh).second)continue;
            auto& job=frame->paints[paint_jobs[e]];const auto& g=draw->Input();
            const bool samples_paint=std::any_of(g.srvs.begin(),g.srvs.end(),[](const auto& s){return s.stage==ds3content::ShaderStage::Pixel && s.shader_register==41;});
            // Final review M1: erase rows gated as the paint rows (t41: the _Bloody families, the only ones with a _PntCol programme,
            // INFERRED), so a family without one does not rescan the shader repository at every erase command.
            if(event.erase){if(!samples_paint){++paint_counts[9];continue;}const auto set=NativeEraseProgramSet(mesh,g.program_set);if(!set){++paint_counts[8];continue;}++paint_counts[6];
                const char* refused{};auto erase=NativeEraseDraw(event,set,g,refused);
                if(!erase){++paint_counts[7];static LONG64 logged{};if(InterlockedIncrement64(&logged)<=8){char row[160]{};
                    sprintf_s(row,"{\"event\":\"native_erase_row_refused\",\"why\":\"%s\",\"recipe\":%u}\n",refused?refused:"",prepared.recipe);Log(row);}continue;}
                job.draws.push_back(std::move(erase));continue;}
            ++paint_counts[0];
            if(!samples_paint){++paint_counts[4];continue;}
            const auto set=NativePaintProgramSet(mesh,prepared.recipe%3,g.program_set);if(!set){++paint_counts[2];continue;}
            const char* refused{};auto paint=NativePaintDraw(*scene,event,set,g,refused);
            if(!paint){++paint_counts[3];static LONG64 logged{};if(InterlockedIncrement64(&logged)<=8){char row[160]{};
                sprintf_s(row,"{\"event\":\"native_paint_row_refused\",\"why\":\"%s\",\"recipe\":%u}\n",refused?refused:"",prepared.recipe);Log(row);}continue;}
            // D-0193 rung 2b: the row's projector depth draw (its Dpt programme) into the job's map before the paint draws; without
            // it the row still paints, unoccluded (the rung 2a picture), and the refusal is counted.
            const char* depth_refused{};auto depth=NativePaintDepthDraw(mesh,prepared.recipe%3,g,depth_refused);
            if(!depth){++paint_counts[5];static LONG64 logged{};if(InterlockedIncrement64(&logged)<=8){char row[160]{};
                sprintf_s(row,"{\"event\":\"native_paint_depth_refused\",\"why\":\"%s\",\"recipe\":%u}\n",depth_refused?depth_refused:"",prepared.recipe);Log(row);}}
            job.draws.push_back(std::move(paint));if(depth)job.depth_draws.push_back(std::move(depth));++paint_counts[1];}};
    uint32_t all_missing_constants{},all_missing_textures{},all_unsupported_palettes{},all_unsupported_layouts{},all_unsupported_samplers{},all_missing_interfaces{},all_unbounded_dynamic{};
    // PREPARE-REDUCTION cP5: blocks shared by the rows of this call (call-local, so nothing outlives the call's inputs): the
    // frame-level blocks per programme object, recipe, stage and slot (forward cbSceneParam, cbPerFrame, velocity
    // cbMatrixWVP, registers without a producer), pose palettes per palette version and padding as well.
    struct CallKey {const void* a{};const void* b{};uint64_t c{},d{};bool operator==(const CallKey&) const=default;};
    struct CallKeyHash {size_t operator()(const CallKey& k) const noexcept {
        return std::hash<const void*>{}(k.a)^(std::hash<const void*>{}(k.b)*31u)^(k.c*1099511628211ull)^(k.d*0x9E3779B97F4A7C15ull);}};
    // W7 TS-4b (WATER-TESS-WAVE1-READY 3.4 TS-4, TESSSCENE-SSMASK-NATIVE 1.3): the view's cbTessScene (hull and domain b5) and
    // cbSceneParam image (domain b8: the domains read VC_ClipPlane), made at the first tessellated row; both null when the
    // camera words are not written. The block's view-projection is the absolute-world one: the camera-relative
    // VC_MatrixViewProj (cbSceneParam 384, the stored rows the ordinary VS applies to position - SC_CameraPos) with each row's
    // w reduced by its xyz dot the eye; the eye is SC_CameraPos, the view vector -forward of the camera pose.
    std::shared_ptr<const std::vector<uint8_t>> tess_scene_block,tess_camera_block;bool tess_view_made=false;
    const auto make_tess_view=[&]{if(tess_view_made)return;tess_view_made=true;
        const auto& c=scene->camera_constants;
        const auto written=[&](uint32_t offset,uint32_t bytes){for(uint32_t at=offset;at<offset+bytes;at+=4)if(c.written[at/4]!=1)return false;return true;};
        if(!written(48,16) || !written(64,16) || !written(384,64))return;
        std::array<float,16> vp{};std::array<float,3> eye{};ds3native::tess::AbsoluteViewProjection(c.bytes.data(),vp,eye);
        const std::array<float,3> forward{scene->camera.camera_pose[8],scene->camera.camera_pose[9],scene->camera.camera_pose[10]};
        auto block=std::make_shared<std::vector<uint8_t>>(0xC0);   // 0xB4 bytes in the declaration's 0xC0
        ds3native::tess::TessScene(vp,eye,forward,scene->simulation_time?scene->simulation_time->now:0.0f,block->data());
        tess_scene_block=std::move(block);
        tess_camera_block=std::make_shared<const std::vector<uint8_t>>(c.bytes.begin(),c.bytes.end());};
    // C4-2: kept draws (main view, not disabled); with frame slots each draw takes one table copy per
    // slot and only a re-encodable draw is kept (frames-in-flight workstream, 2026-10-18).
    const bool keep=!offscreen && !native_kept_draws_disabled;
    const uint32_t kept_slots=std::max(renderer.FrameSlots(),1u);
    // B5: the snow inputs this prepare's rows write (the stamp a kept row that holds FC_SnowRate compares).
    std::optional<NativeKeptSnow> current_snow;
    if(keep && scene->snow_rows){NativeKeptSnow stamp{{scene->snow_rows->scene_rate,scene->snow_rows->model_builtin}};
        std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
        stamp.generation=banks?banks->RecordGeneration():0;   /* I-0141 tranche 23: the snow rate fades */ stamp.banks=std::move(banks);current_snow=std::move(stamp);}
    std::unique_lock<std::mutex> kept_hold;std::vector<uint64_t> model_versions;
    auto& kept=native_kept_draws;uint64_t kept_serial{};
    if(keep){kept_hold=std::unique_lock(kept.lock);kept_serial=++kept.prepares;model_versions.assign(scene->models.size(),0);
        NativeKeptEpoch epoch{scene->sampler_bank,scene->sampler_bank->generation,gpu.device.Get(),scene->pixel_policy,scene->category_scalar,
            scene->default_param_row,scene->velocity.has_value(),bool(scene->simulation_time),current_snow.has_value(),scene->taa && NativeTaaReactive()};
        const auto resets=native_kept_draw_resets.load(std::memory_order_acquire);
        if(!kept.epoch || !(*kept.epoch==epoch) || kept.resets_seen!=resets){
            if(kept.epoch)++kept.outcomes[4];
            kept.draws.clear();kept.models.clear();kept.epoch=std::move(epoch);kept.resets_seen=resets;}}
    // C4-2: the model's version: its inputs compared with the previous prepare's, a new number when they differ.
    const auto model_version_compute=[&](size_t index)->uint64_t{
        auto& version=model_versions[index];if(version)return version;
        const auto& model=scene->models[index];NativeKeptModelInputs inputs{model.meshes.items,model.geometry,model.pose,model.previous_pose,model.streams,
            model.engine_bytes,model.forward_light,model.region_ibl,model.placement,model.cloth_world,model.ordinary_category,model.draw_colour,model.texture_scroll,
            model.geometry_fades.intents,model.animation!=0,model.phantom,model.phantom_kind,model.phantom_fade,model.asm_part};
        auto& record=kept.models[{model.entity,model.model}];record.last_seen=kept_serial;
        const int changed=record.version?NativeKeptModelDifference(record.inputs,inputs):-1;if(changed>=0)++kept.model_changes[changed];
        if(!record.version || changed>=0){record.inputs=std::move(inputs);record.version=++kept.versions;}
        return version=record.version;};
    const auto hide_fragment=chair_hunt::Fragment();
    struct PrepareChunk {std::shared_ptr<NativeModelFrameInput> frame=std::make_shared<NativeModelFrameInput>();
        uint32_t missing_constants{},missing_textures{},unsupported_palettes{},unsupported_layouts{},unsupported_samplers{},missing_interfaces{},unbounded_dynamic{};
        std::map<int32_t,NativeAsmOutcome> asm_outcomes;std::map<std::string,NativeAsmOutcome> family_outcomes;
        std::array<uint64_t,9> outcomes{};   // the kept draws' outcomes (NativeKeptDraws::outcomes)
        struct KeptOp {NativeDryRowKey key;NativeKeptDraw entry;bool insert{};};std::vector<KeptOp> kept_ops;   // applied in row order after the chunks
        std::vector<std::pair<size_t,std::shared_ptr<const OwnedNativeModelDraw>>> paints;   // D2 rows, painted in row order after the chunks
        std::vector<size_t> forward_rows;   // I-0113 tranche 15: each forward draw's prepared row (SortNativeForwardRows)
        std::vector<std::pair<size_t,bool>> popin_rows;};   // I-0100 T0: {model, drawn} of each main-view row (popin-observe.flag)
    const bool popin=PopinObserve() && !offscreen;
    const bool asm_summary_open=NativeAsmSummaryOpen();   // I-0120 B2: the part tally only while the summary (40 rows) can still log, read once per prepare
    std::vector<uint32_t> occurrences(scene->prepared_meshes.size());
    // I-0075: one chunk of rows [first_row, end_row) into its own outputs; what the rows share (the kept draws, the model
    // versions, the occurrences, the scene) is only read here.
    const auto run_rows=[&](PrepareChunk& out,size_t first_row,size_t end_row){
        auto& chunk_frame=out.frame;auto& missing_constants=out.missing_constants;auto& missing_textures=out.missing_textures;
        auto& unsupported_palettes=out.unsupported_palettes;auto& unsupported_layouts=out.unsupported_layouts;auto& unsupported_samplers=out.unsupported_samplers;
        auto& missing_interfaces=out.missing_interfaces;auto& unbounded_dynamic=out.unbounded_dynamic;
        auto& asm_outcomes=out.asm_outcomes;auto& family_outcomes=out.family_outcomes;auto& chunk_outcomes=out.outcomes;auto& chunk_kept_ops=out.kept_ops;
        size_t row_index{};
        const auto paint_row=[&](const NativeSelectedModel&,const NativeSelectedMesh&,const NativeSelectedScene::PreparedMesh&,const std::shared_ptr<const OwnedNativeModelDraw>& draw){
            out.paints.emplace_back(row_index,draw);};
    std::unordered_map<CallKey,std::shared_ptr<const ds3content::ShaderRegisterBlock>,CallKeyHash> call_blocks;
    const auto share=[&](const CallKey& key,const auto& produce){auto& entry=call_blocks[key];
        if(!entry)entry=std::make_shared<const ds3content::ShaderRegisterBlock>(produce());return entry;};
    // C4-1 frame registers (VS b0, b8, b9; PS b8, b9): the block of a row's programme, from the scene and view tables or made
    // once per call; null when the scene has none (the row is then incomplete). C4-2: the same production serves a kept draw.
    const auto frame_block_of=[&](const NativeSelectedMesh& mesh,const NativeSelectedScene::PreparedMesh& prepared,size_t stage,uint32_t slot,
        bool velocity_draw,bool forward_draw)->SharedRegisterBlock{
        if(slot==8 && !(stage && forward_draw))return NativeMeshSceneBlock(*scene,mesh,prepared.recipe,stage);
        if(slot==8)return share({mesh.programs.get(),nullptr,prepared.recipe,(1u<<16)|(stage<<8)|slot},[&]{
            // 1116 forward PS cbSceneParam: the camera words, plus the clustered light-grid words FC_FrustumSliceScale/Bias
            // (884/888) and FC_LightTile_Width/NumCellX (896/900) of the light grid (I-0107, NativeForwardGridWords).
            auto bytes=scene->camera_constants.bytes;auto written=scene->camera_constants.written;
            NativeForwardGridWords(*scene,offscreen,bytes,written);   // I-0107
            const auto ranges=NativeShaderWordRanges(bytes,written);
            return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbSceneParam");});
        if(slot==9 && scene->simulation_time)return share({mesh.programs.get(),nullptr,prepared.recipe,(2u<<16)|(stage<<8)|slot},[&]{
            // C3: the vertex stage reads FC_GlobalTime too (FlowmapVertexOffset)
            std::array<uint8_t,0xE0> bytes{};std::array<uint8_t,0xE0/4> written{};
            ds3prod::ProduceFrameTime(scene->simulation_time->now,bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());
            if(scene->per_frame_rows)ds3prod::ProducePerFrame(scene->simulation_time->now,scene->per_frame_rows->a.data(),scene->per_frame_rows->b.data(),
                scene->per_frame_rows->blend,bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());
            ProduceNativePerFrameGlobals(bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());
            ProduceNativeSnowRows(*scene,bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());   // I-0105
            const auto ranges=NativeShaderWordRanges(bytes,written);
            return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbPerFrame");});
        if(!stage && slot==0 && velocity_draw)return share({mesh.programs.get(),nullptr,prepared.recipe,(3u<<16)|(stage<<8)|slot},[&]{
            // 1186: Velo VS b0 {VC_MatrixWVP, VC_PrevMatrixWVP}
            const ds3content::ShaderConstantRange range{0,scene->velocity->matrices};
            return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,{&range,1},"cbMatrixWVP");});
        if(!stage && slot==0)return NativeMeshViewBlock(*scene,mesh,prepared.recipe);
        return share({mesh.programs.get(),nullptr,prepared.recipe,(5u<<16)|(stage<<8)|slot},[&]{   // a register without a producer
            return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,{});});};
    // C4-1: a frame binding's block becomes the view's canonical block of its kind when it is the longest.
    const auto note_frame_block=[&](const SharedRegisterBlock& block,uint32_t stage,uint32_t slot,bool velocity_draw,bool forward_draw){
        const auto root=NativeModelFrameRootOf(stage,slot);
        const auto kind=root==FrameVsView?(velocity_draw?FrameBlockVelocity:FrameBlockView):
            root==FramePsScene && forward_draw?FrameBlockForwardScene:(root==FrameVsPerFrame || root==FramePsPerFrame)?FrameBlockPerFrame:FrameBlockScene;
        std::shared_ptr<const std::vector<uint8_t>> payload(block,&block->bytes);
        auto& canonical=chunk_frame->frame_blocks[kind];if(!canonical || canonical->size()<payload->size())canonical=payload;
        return payload;};
    for(row_index=first_row;row_index<end_row;++row_index){const auto& prepared=scene->prepared_meshes[row_index];
        if(!hide_fragment.empty() && !offscreen){const auto& hidden_model=scene->models.at(prepared.model);const auto& hidden_mesh=hidden_model.meshes.at(prepared.mesh);
            if(hide_fragment[0]==L'@'){chair_hunt::Near(hide_fragment,hidden_model.model,hidden_model.placement,hidden_mesh.material.get(),prepared.mesh);}
            else if(hide_fragment[0]==L'#'){if(chair_hunt::NamedModel(hide_fragment,hidden_model.model))continue;}
            else if(hidden_mesh.material && chair_hunt::Hidden(hidden_mesh.material->texture_inputs,hide_fragment))continue;}
        kprofile::SwitchIfActive(kprofile::PrepRowSetup);kprofile::AddIfActive(kprofile::PrepRows);
        const auto& model=scene->models.at(prepared.model);const auto& mesh=model.meshes.at(prepared.mesh);
        bool asm_drawn=false;
        struct AsmTally {std::map<int32_t,NativeAsmOutcome>& map;int32_t part;const bool& drawn;std::array<const uint32_t*,6> counters;std::array<uint32_t,6> before;
            std::map<std::string,NativeAsmOutcome>& families;std::string family;
            ~AsmTally(){if(part<0)return;for(auto* o:{&map[part],&families[family]}){++o->prepared;if(drawn)++o->drawn;for(size_t i=0;i<6;++i)if(*counters[i]!=before[i])++o->fail[i];}}
        } asm_tally{asm_outcomes,asm_summary_open && model.asm_part<1000?model.asm_part:-1,asm_drawn,{&missing_constants,&missing_textures,&unsupported_palettes,&unsupported_layouts,&unsupported_samplers,&missing_interfaces},
            {missing_constants,missing_textures,unsupported_palettes,unsupported_layouts,unsupported_samplers,missing_interfaces},
            family_outcomes,asm_summary_open && model.asm_part>=0 && model.asm_part<1000?NativeFamilyKey(*mesh.programs):std::string{}};
        struct PopinRow {std::vector<std::pair<size_t,bool>>* rows;size_t model;const bool& drawn;~PopinRow(){if(rows)try{rows->emplace_back(model,drawn);}catch(...){}}}   // review L3
            popin_row{popin && prepared.engine_pass!=1 && prepared.engine_pass!=0x34?&out.popin_rows:nullptr,prepared.model,asm_drawn};   // I-0100 T0
        // C4-2: the row's kept draw, when its model version, tiling values and instance blocks are unchanged; its frame
        // registers are produced for this frame.
        NativeDryRowKey kept_key{};NativeKeptDraw* kept_entry{};uint64_t row_model_version{};const OwnedNativeModelDraw* kept_base{};
        std::optional<std::array<float,4>> row_velocity_scale;std::optional<NativeKeptSnow> row_snow;   // B5: what a fresh draw wrote
        std::array<const SharedRegisterBlock*,2> row_instances{};   // I-0074: the row's, made with it
        bool fade_mark{};   // I-0140: this fresh row draws into the TAA's fade mask this frame
        // W7 TS-2b: a tessellated row binds per-view hull/domain blocks outside the kept key: never kept.
        const bool tess_row=NativeTessellatedRecipe(prepared.recipe);
        if(keep && !tess_row){kprofile::SwitchIfActive(kprofile::PrepKept);
            kept_key={model.entity,model.model,prepared.mesh,prepared.fade,prepared.engine_pass,prepared.recipe,occurrences[row_index]};   // I-0075: a repeated row's occurrence, counted in row order first
            auto found=kept.draws.find(kept_key);if(found!=kept.draws.end())kept_base=found->second.draw.get();   // I-0090: the state a rebuild may take
            row_model_version=model_versions[prepared.model];
            for(size_t stage=0;stage<2;++stage){row_instances[stage]=prepared.instances[stage];
                if(!row_instances[stage])throw std::logic_error("native pass row without its instance blocks");}   // review L3: every row is made with them
            if(found==kept.draws.end()){++chunk_outcomes[1];kprofile::AddIfActive(kprofile::PrepKeptNew);}
            else if(found->second.model_version!=row_model_version){++chunk_outcomes[2];kprofile::AddIfActive(kprofile::PrepKeptModel);}
            else if(!(found->second.tiling.buffer_register==prepared.tiling.buffer_register && found->second.tiling.stages==prepared.tiling.stages &&
                      found->second.tiling.values==prepared.tiling.values) || !NativeSameRegisterBlock(found->second.instance_blocks[0],*row_instances[0]) ||
                    !NativeSameRegisterBlock(found->second.instance_blocks[1],*row_instances[1])){++chunk_outcomes[3];kprofile::AddIfActive(kprofile::PrepKeptRow);}
            else if(!found->second.draw->ReEncodable(kept_slots)){++chunk_outcomes[3];kprofile::AddIfActive(kprofile::PrepKeptRow);}   // review: a draw kept under another slot count is rebuilt
            // B5: a row that wrote the velocity scale or the snow rate, rebuilt when what it wrote changed.
            else if((found->second.velocity_scale && !(scene->velocity && NativeSameBits(*found->second.velocity_scale,scene->velocity->scale))) ||
                    (found->second.snow && !(current_snow && *found->second.snow==*current_snow))){++chunk_outcomes[3];kprofile::AddIfActive(kprofile::PrepKeptRow);}
            // I-0113 (i113-s8-wheel-native: a kept row is not prepared again, so "!name" described nothing): while "!name" is set, the
            // named models' rows are rebuilt (witness tooling).
            else if(hide_fragment.size()>1 && hide_fragment[0]==L'!' && !offscreen && chair_hunt::Matches(hide_fragment,model.model)){++chunk_outcomes[3];}
            else{kept_entry=&found->second;kept_entry->last_used=kept_serial;
                const bool velocity_draw=prepared.engine_pass==0x34;
                const bool water_draw=prepared.engine_pass==0x24 || prepared.engine_pass==0x27;   // WATER-TESS-WAVE1-READY WT-3/WT-4
        const bool forward_draw=water_draw || prepared.engine_pass==0x29 || prepared.engine_pass==0x1F || prepared.engine_pass==0x21 || prepared.engine_pass==0x22;
                // The frame registers first (review: they are outside the key, so a refused one is a rebuild).
                bool frames_ready=true;
                for(const auto& binding:kept_entry->draw->Input().constants){if(!binding.frame)continue;
                    const uint32_t stage=binding.stage==ds3content::ShaderStage::Pixel?1u:0u;
                    const auto block=frame_block_of(mesh,prepared,stage,binding.shader_register,velocity_draw,forward_draw);
                    if(!block || block->status!=ds3content::ShaderConstantStatus::Ready || !block->required || block->bytes.size()!=binding.Payload().size()){frames_ready=false;break;}
                    note_frame_block(block,stage,binding.shader_register,velocity_draw,forward_draw);}
                if(!frames_ready){++chunk_outcomes[7];kprofile::AddIfActive(kprofile::PrepKeptFrameRefusals);kept_entry=nullptr;}   // built again below
                else{
                    {if(!velocity_draw && !forward_draw)paint_row(model,mesh,prepared,kept_entry->draw);   // D2
                        (water_draw?(prepared.engine_pass==0x24?chunk_frame->water_depth_draws:chunk_frame->water_draws):velocity_draw?chunk_frame->velocity_draws:forward_draw?chunk_frame->forward_draws:chunk_frame->draws).push_back(kept_entry->draw);
                        if(forward_draw && !water_draw)out.forward_rows.push_back(row_index);
                        chunk_frame->forward_early+=prepared.engine_pass==0x1F;}asm_drawn=true;
                    ++chunk_outcomes[0];kprofile::AddIfActive(kprofile::PrepKeptHits);kprofile::AddIfActive(kprofile::PrepDraws);continue;}}
            kprofile::SwitchIfActive(kprofile::PrepRowSetup);}
        if(!mesh.layout->vertex_elements_available){++unsupported_layouts;continue;}
        bool supported_layout=true;
        for(const auto& element:mesh.layout->vertex_elements){const auto width=NativeVertexElementBytes(element.format);
            // 1063: slot 1 is the second buffer of a two-stream mesh (stride2).
            const uint32_t stride=element.input_slot==1?mesh.slice.stride2:mesh.slice.stride;
            if(!width || element.input_slot>1 || !stride || element.input_class!=D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA ||
               element.instance_step_rate || element.byte_offset>stride || width>stride-element.byte_offset)
                supported_layout=false;
        }
        if(!supported_layout){++unsupported_layouts;continue;}
        NativeModelDrawInput input;input.geometry=mesh.geometry;if(keep && !tess_row)input.table_copies=kept_slots;
        // PREPARE-REDUCTION cP1: the recipe's shared programme set (no per-row bytecode and element copies; it is made or throws).
        input.program_set=NativeProgramSetOf(*mesh.programs,prepared.recipe,mesh.layout->vertex_elements);
        input.sampler_bank=scene->sampler_bank.get();input.sampler_bank_generation=scene->sampler_bank->generation;
        const auto& draw_vertex_shader=input.program_set->vertex;
        const auto& draw_elements=input.program_set->elements;
        // I-0113 gap 14: a model particle's palettes are its mesh's (NativeEffectModel::MeshPose, 64B30); its pose stands for both versions.
        const NativeModelPoseSnapshot* draw_pose=model.pose.get();const NativeModelPoseSnapshot* draw_previous=model.previous_pose.get();
        if(model.effect)draw_pose=draw_previous=model.effect->MeshPose(mesh.current.ordinal);
        ds3content::ModelPoseBinding pose_binding;
        if(draw_pose && draw_pose->palettes && mesh.pose_policy)
            pose_binding=ds3content::SelectOrdinaryModelPoseBinding(*draw_pose->palettes,
                mesh.pose_policy->cloth_enabled,(mesh.pose_policy->flags&0x4000u)!=0);
        // 1185: with the cloth flag now produced (+0x22), 64340's cloth route asks for the auxiliary palette, which
        // native poses do not own for ChrAsm parts (Native1185: parts 2 and 3 refused on palettes). Explicit open
        // policy: such a mesh binds the ordinary current route, as before the flag existed; with a bound simulated
        // copy the cloth VS reads VC_World only, without one the mesh is skinned by its palette.
        if(pose_binding.kind==ds3content::ModelPoseBindingKind::Unavailable && draw_pose && draw_pose->palettes &&
           mesh.pose_policy && mesh.pose_policy->cloth_enabled)
            pose_binding=ds3content::SelectOrdinaryModelPoseBinding(*draw_pose->palettes,false,(mesh.pose_policy->flags&0x4000u)!=0);
        // 1186: a Velo draw (row 0x34) binds b6 cbPrevObjMatrix (64340: the pose ring's slot index-1) through the same route
        // on the previous pose version; a version that would take another route leaves b6 unavailable.
        const bool velocity_draw=prepared.engine_pass==0x34;
        ds3content::ModelPoseBinding previous_binding;
        if(velocity_draw && draw_previous && draw_previous->palettes && mesh.pose_policy){
            previous_binding=ds3content::SelectOrdinaryModelPoseBinding(*draw_previous->palettes,
                mesh.pose_policy->cloth_enabled,(mesh.pose_policy->flags&0x4000u)!=0);
            if(previous_binding.kind==ds3content::ModelPoseBindingKind::Unavailable && mesh.pose_policy->cloth_enabled)
                previous_binding=ds3content::SelectOrdinaryModelPoseBinding(*draw_previous->palettes,false,(mesh.pose_policy->flags&0x4000u)!=0);
            if(previous_binding.kind!=pose_binding.kind)previous_binding={};}
        const auto& vertex_reads=mesh.programs->ReadsOf(prepared.recipe,0);
        const bool dynamic_palettes=vertex_reads.dynamic.contains(5) || vertex_reads.dynamic.contains(12);
        // 1120: the original normal route (64340 -> GXFlverModelDC +0x28 = 1A9250) binds only VS b5; VS b12
        // (cbClothCancelObjMatrix, read unconditionally by the skinned SAT_Equip VS) keeps the binding left by the
        // preceding draw, which for a cloth-posed model is its cloth draw's cancel (+0x38 = 1A9340: b5 and b12).
        // Explicit policy: a current-route draw on a cloth pose binds that same pose's cancel palette.
        bool cancel_retained=false;
        if(pose_binding.kind==ds3content::ModelPoseBindingKind::Current && !pose_binding.cancel && vertex_reads.dynamic.contains(12) &&
           draw_pose->palettes->cloth && draw_pose->palettes->cancel.Available() &&
           draw_pose->palettes->cancel.rows.size()==draw_pose->palettes->row_count){
            pose_binding.cancel=&draw_pose->palettes->cancel;cancel_retained=true;}
        bool palette_indices_valid=false;uint32_t palette_max_index{};
        if(dynamic_palettes && pose_binding.object && pose_binding.cancel){
            std::span<const uint8_t> vertices;
            if(mesh.geometry.stream_generation && model.streams){
                for(const auto& candidate:model.streams->streams)
                    if(candidate.mesh==mesh.current.patch_index && candidate.resource.Get()==mesh.geometry.vertex_resource.Get()){
                        vertices=candidate.bytes;break;
                    }
            }else if(!mesh.geometry.stream_generation && model.geometry){
                const auto count=uint64_t(mesh.slice.vertex_count)*mesh.slice.stride;
                if(mesh.slice.vertex_offset<=model.geometry->bytes.size() && count<=model.geometry->bytes.size()-mesh.slice.vertex_offset)
                    vertices=std::span<const uint8_t>(model.geometry->bytes).subspan(mesh.slice.vertex_offset,static_cast<size_t>(count));
            }
            const ds3content::ModelPaletteIndexInputs indices{draw_elements,vertices,mesh.slice.vertex_count,mesh.slice.stride,
                0,static_cast<uint32_t>(pose_binding.object->rows.size()),static_cast<uint32_t>(pose_binding.cancel->rows.size())};
            LARGE_INTEGER validate_start{},validate_end{};QueryPerformanceCounter(&validate_start);
            kprofile::SwitchIfActive(kprofile::PrepPaletteCheck);
            // PREPARE-REDUCTION cP4: the check is a pure function of the programme recipe (its bytecode and elements), the
            // vertex bytes and the row counts; the vertex bytes are an immutable authored storage (storage generation and
            // offset) or an immutable physical stream version (stream generation and patch), or none (a cloth stream: the
            // span is empty). Kept per Programs with the whole key.
            const NativeModelLayout::Programs::PaletteCheckKey palette_key{prepared.recipe,vertices.empty()?0:mesh.geometry.stream_generation,
                vertices.empty()?0:mesh.geometry.storage_generation,vertices.empty()?0u:mesh.current.patch_index,vertices.empty()?0ull:mesh.slice.vertex_offset,
                mesh.slice.vertex_count,mesh.slice.stride,indices.object_rows,indices.cancel_rows,vertices.size()};
            std::optional<NativeModelLayout::Programs::PaletteCheck> kept_check;
            {const std::lock_guard hold(mesh.programs->palette_lock);
                for(const auto& entry:mesh.programs->palette_checks)if(entry.first==palette_key){kept_check=entry.second;break;}}
            if(!kept_check){
                kprofile::AddIfActive(kprofile::PrepPaletteChecks);
                const auto checked=ds3content::ValidateModelPaletteIndices(draw_vertex_shader,indices);
                const NativeModelLayout::Programs::PaletteCheck fresh{checked.status,checked.max_index};
                {const std::lock_guard hold(mesh.programs->palette_lock);auto& checks=mesh.programs->palette_checks;
                    std::erase_if(checks,[&](const auto& entry){return entry.first==palette_key;});
                    if(checks.size()>=256)checks.erase(checks.begin());
                    checks.push_back({palette_key,fresh});}
                kept_check=fresh;}
            kprofile::AddIfActive(kprofile::PrepPaletteInRange,kept_check->status==ds3content::ModelPaletteIndexStatus::InRange?1:0);kprofile::SwitchIfActive(kprofile::PrepRowSetup);
            QueryPerformanceCounter(&validate_end);native_prep_ticks[0]+=validate_end.QuadPart-validate_start.QuadPart;
            palette_indices_valid=kept_check->status==ds3content::ModelPaletteIndexStatus::InRange;palette_max_index=kept_check->max_index;
        }
        // 1120 witness: how far the retained cancel rows this mesh can address are from identity (0 = the policy
        // equals an identity cancel for this mesh).
        if(cancel_retained && palette_indices_valid){static LONG64 retained_samples{};
            if(InterlockedIncrement64(&retained_samples)<=48){const auto& rows=pose_binding.cancel->rows;float deviation{};
                const size_t addressed=std::min<size_t>(rows.size(),size_t(palette_max_index)+1);
                for(size_t r=0;r<addressed;++r)for(size_t k=0;k<12;++k){const float identity=(k==0 || k==5 || k==10)?1.0f:0.0f;
                    deviation=std::max(deviation,std::fabs(rows[r][k]-identity));}
                char row[320]{};sprintf_s(row,"{\"event\":\"native_cloth_cancel_retained\",\"asm_part\":%d,\"mesh\":%u,\"family\":\"%.100s\",\"max_index\":%u,\"rows_addressed\":%zu,\"cancel_rows\":%zu,\"max_identity_deviation\":%g}\n",
                    model.asm_part,mesh.current.ordinal,NativeFamilyKey(*mesh.programs).c_str(),palette_max_index,addressed,rows.size(),deviation);Log(row);}}
        // C4-2: a kept draw holds what it references (its mesh set with programmes, materials and textures, the geometry
        // storage, the stream version, the sampler bank named by pointer), not the scene.
        // A null owner is refused by the draw (Native1430: a model without a stream version), so only present ones are held.
        if(keep){for(const std::shared_ptr<const void>& owner:{std::shared_ptr<const void>(model.meshes.items),std::shared_ptr<const void>(model.geometry),
                std::shared_ptr<const void>(model.streams),std::shared_ptr<const void>(scene->sampler_bank)})if(owner)input.cpu_owners.push_back(owner);}
        else input.cpu_owners.push_back(scene);
        input.cpu_owners.push_back(prepared.scroll);
        {const auto& six=SceneRenderer::ModelTargetFormats();input.render_target_formats={};std::copy(six.begin(),six.end(),input.render_target_formats.begin());}
        input.depth_format=SceneRenderer::ModelDepthFormat();
        input.shape.envelope=ModelPassEnvelope::GBuffer6;   // W0a: the producer names its target set
        input.sample_mask=UINT_MAX;
        // 140140850 (G-buffer pass 6): |8, then material+0x24 bit 0x800 (SSS skin) |4, else bit 0x80 |2.
        // GXSssBlurX/Y read stencil bit 4 only.
        // DECALS-WAVE1 D1 (140850 L229-243): bit 0, the decal receiver, only for a map or object part (5A120 kinds 3 and 4: not a
        // character) whose material does not disable decals (MTD g_GX_bDisableDecal, policy.disable_decal); 51AA0 tests EQUAL 1.
        {const bool receiver=!model.character && !(mesh.programs && mesh.programs->policy && mesh.programs->policy->disable_decal);
            uint8_t stencil=receiver?9:8;if(mesh.pose_policy){if(mesh.pose_policy->flags&0x800u)stencil|=4;else if(mesh.pose_policy->flags&0x80u)stencil|=2;}
            input.stencil_reference=stencil;}
        // The six-output G-buffer state; its blend by the material's blend mode below (I-0140 tranche 6, the Decal rows among
        // them); the forward rows' states after it.
        // 140064610 (mode 0, the ordinary draw context): the face set's class through 14473C9F8 row 0 {0,1,2}; the
        // mirrored row 1 {0,2,1} swaps 1 and 2. Measured (Native1184): map and armour face sets carry class 2 and
        // need back-face culling (with 2 = front they rendered inside out), so 0 = none, 1 = front, 2 = back;
        // two-sided cards (CullBackfaces False) keep their back faces.
        input.rasterizer.FillMode=D3D12_FILL_MODE_SOLID;
        input.rasterizer.CullMode=mesh.slice.cull_class==0?D3D12_CULL_MODE_NONE:mesh.slice.cull_class==1?D3D12_CULL_MODE_FRONT:D3D12_CULL_MODE_BACK;
        // I-0113 gap 12 (Flame Fan's fan, s08280, CullBackfaces in its FLVER): a model particle's row of that table comes from its draw
        // arguments (64610: +0x180 bit 9 or +0x184 = 2, row 2, every class drawn two-sided; bit 7 or +0x184 = 1, row 1, mirrored).
        if(model.effect){const auto& s=model.effect->submission;
            const uint32_t row=(s.arg_cull>>9&1) || s.arg_cull_mode==2?2u:(s.arg_cull>>7&1) || s.arg_cull_mode==1?1u:0u;
            const uint32_t cull=row==2?0u:row==1 && (mesh.slice.cull_class==1 || mesh.slice.cull_class==2)?3u-mesh.slice.cull_class:mesh.slice.cull_class;
            input.rasterizer.CullMode=cull==0?D3D12_CULL_MODE_NONE:cull==1?D3D12_CULL_MODE_FRONT:D3D12_CULL_MODE_BACK;}
        // Phantoms (Native1878: an NPC sign phantom shows its inner surfaces): the first 48 phantom-row draws (0x21, 0x22, 0x29) of
        // character models by name and mesh: the face set's class, the category, the model's asm part, the instance matrix
        // determinant (a mirrored placement), and the draw's state bytes +0x84/+0x85/+0x87 of the group.
        static const bool phantom_diagnostics=ModuleFlag(L"light-observe.flag");   // review: diagnostics only with light-observe.flag
        if(phantom_diagnostics && !offscreen && (prepared.engine_pass==0x21 || prepared.engine_pass==0x22 || prepared.engine_pass==0x29)){
            const auto name=NativeModelDataName(model.model);
            if(model.asm_part>=0 || (!name.empty() && (name[0]=='c' || name[0]=='C'))){static std::mutex phantom_cull_lock;static std::set<std::tuple<uintptr_t,uint32_t,uint32_t>> phantom_cull_seen;bool fresh{};
                {const std::lock_guard hold(phantom_cull_lock);fresh=phantom_cull_seen.size()<48 && phantom_cull_seen.emplace(model.model,mesh.current.ordinal,prepared.engine_pass).second;}
                if(fresh){uint8_t bytes[4]{};Read(model.group+0x84,bytes,4);
                    std::array<float,12> rows{};if(model.pose && !model.pose->matrices.empty())rows=model.pose->matrices.front();
                    const float det=rows[0]*(rows[5]*rows[10]-rows[6]*rows[9])-rows[1]*(rows[4]*rows[10]-rows[6]*rows[8])+rows[2]*(rows[4]*rows[9]-rows[5]*rows[8]);
                    char row[320]{};sprintf_s(row,"{\"event\":\"native_phantom_cull\",\"model\":\"%.40s\",\"mesh\":%u,\"pass\":%u,\"cull_class\":%u,\"category\":%d,\"asm_part\":%d,\"det\":%g,\"group84\":[%u,%u,%u,%u],\"entity\":%llu}\n",
                        name.c_str(),mesh.current.ordinal,prepared.engine_pass,static_cast<unsigned>(mesh.slice.cull_class),model.ordinary_category?int(*model.ordinary_category):-1,model.asm_part,det,bytes[0],bytes[1],bytes[2],bytes[3],static_cast<unsigned long long>(model.entity));Log(row);}}}
        input.rasterizer.FrontCounterClockwise=TRUE;input.rasterizer.DepthClipEnable=TRUE;
        input.depth_stencil.DepthEnable=TRUE;input.depth_stencil.DepthWriteMask=mesh.programs->policy->disable_depth_write?
            D3D12_DEPTH_WRITE_MASK_ZERO:D3D12_DEPTH_WRITE_MASK_ALL;
        input.depth_stencil.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;input.depth_stencil.StencilEnable=TRUE;
        input.depth_stencil.StencilReadMask=input.depth_stencil.StencilWriteMask=255;
        input.depth_stencil.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_REPLACE,D3D12_COMPARISON_FUNC_ALWAYS};
        input.depth_stencil.BackFace=input.depth_stencil.FrontFace;
        for(auto& target:input.blend.RenderTarget){target.SrcBlend=target.SrcBlendAlpha=D3D12_BLEND_ONE;
            target.DestBlend=target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOp=target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
            target.LogicOp=D3D12_LOGIC_OP_NOOP;target.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;}
        // I-0140 tranche 6 (the Decal materials, ROADMAP workstream 5 step 2; READ, evidence/I-0140/ghidra-decal-class8/): a G-buffer
        // row's blend is its material's blend parameter's (140200840, called by 140850's upload 2E5C0): mode m (the parameter's +0x10;
        // a per-instance override found by its name's CRC is not modelled) through the table 14473E5C0 row 0 {0,0,1,2,3,4}; 0 is the
        // bank's +0x08 (no blend), any other i is i+0xF (5A120 sets args+0x200 bit 16 for the Gbuf pass), the bank entries 1400D21F0
        // builds: 16 SRC_ALPHA/INV_SRC_ALPHA, 17 DEST_COLOR/ZERO, 18 SRC_ALPHA/ONE, 19 the same reverse-subtracted, the alpha ONE/ZERO
        // added, every target written RGB only (mask 7): the bank's offsets 0x88 to 0xA0, the observed table NativeEffectBankBlend
        // reads; the rule is NativeMaterialBlendToken's (the kind 0xFF and row 0 1A86C0 leaves after 5A120, bit 16), the table read
        // flat as 140200840 reads it (a mode past 5 reads the next row). The class (8 for a Decal, 142130)
        // plays no part; a non-Decal material with a nonzero mode is class 2 (MTD +0x111, policy.transparent), drawn by the forward
        // rows, so in practice the Decal rows (engine pass 6) take it. Depth: 140850's pair +0x860 (written) or, with MTD +0x111,
        // +0x868 (not written; 1400FF630); the MTD's g_GX_bDisableDepthWrite (+0x81, above), which 140850 does not read, stays the
        // native's explicit policy. The rows keep the walk's order within each engine pass (5, 6, 7 in turn, as 57A20 records
        // them); the original sorts them by its pass key (BF4C0, 1297E0): two blended Decals over one another may composite in
        // another order (not modelled).
        if(prepared.engine_pass>=5 && prepared.engine_pass<=7 && mesh.programs && mesh.programs->policy){   // the G-buffer rows
            if(const uint32_t token=NativeMaterialBlendToken(0xFF,0,1u<<16,mesh.programs->policy->blend_mode);token>8){D3D12_BLEND_DESC bank{};
                if(ds3native::NativeEffectBankBlend(token,bank))input.blend=bank;}   // entries 16 to 19: offsets 0x88 to 0xA0
            if(mesh.programs->policy->transparent)input.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;}
        // 1114 forward pass 0x29: one lit HDR target after deferred lighting, depth
        // tested, written by the phantom rows 0x21/0x22 and by Begin's rule for an ordinary row (below), no stencil. An ordinary row's blend is the
        // original's rule (I-0113 tranche 10b, NativeOrdinaryForwardBlend; fading rows too since 10d); where it does not resolve, and for the phantom rows,
        // the explicit open policy for the MTD blend mode: 0 opaque, 2 alpha blend (source alpha, inverse source alpha); others refused.
        // 1181: the 0x1F row (alpha-tested forward meshes) takes the same forward state, ahead of the 0x29 draws (prepared in
        // row order), its depth by the same rule.
        const bool water_draw=prepared.engine_pass==0x24 || prepared.engine_pass==0x27;   // WATER-TESS-WAVE1-READY WT-3/WT-4
        const bool forward_draw=water_draw || prepared.engine_pass==0x29 || prepared.engine_pass==0x1F || prepared.engine_pass==0x21 || prepared.engine_pass==0x22;
        if(forward_draw){
            const auto blend_mode=mesh.programs->policy->blend_mode;
            // I-0105: an FFX model particle the blend rule below leaves unresolved takes the blend the effect context had bound at
            // 636F0's entry (i105-bolt-ref-11: additive 0x20); its raster stays its material's (INFERRED: the original's raster varied).
            D3D12_BLEND_DESC effect_blend{};D3D12_DEPTH_STENCIL_DESC effect_depth{};
            // I-0113 gap 17: the mesh loop rebinds both states of an E03 particle. The blend is its material's blend-mode parameter's
            // (NativeEffectModelBlend). The depth is Begin's (140140680:248-268): for a material whose class has none of the bits 0x10D
            // (or with the arguments' bit 17), 0x1E0 (tested, written) when args+0x188 is 1, or nonzero (29ADF0's -(command[0] != 0))
            // with depth write allowed, else 0x1E8 (tested, unwritten); for another class the forward pair [[GXRS+0x4DE8]+0x860] when
            // args+0x188 is nonzero and the MTD's +0x111 clear, else +0x868 (1400FF630 makes them the bank's 0x1E0 and 0x1E8 with a
            // stencil written, STUDY-TRANSLUCENT-CHAMELEON §6; the crystals and the flask, class 1, take +0x860: written LESS_EQUAL as
            // i113-t9-crystal-oracle2 and -flask-oracle2 saw). +0x111 is clear for the classes 1, 4 and 0x100 (142130 returns 2
            // otherwise; a Decal, class 8 or 0x10, may carry it: the G-buffer rows take it, 0x10 its forward rows). Not modelled: the pair's stencil (the forward rows keep it off),
            // Begin's class being the one its previous call cached (one draw late when a material's class changes). Args+0x18C bit 10
            // (the third condition) is clear on this path (5A120 sets it only in preparation 1).
            const uint32_t material_blend=model.effect?NativeEffectModelBlend(*model.effect,blend_mode):0;
            const uint32_t blend_token=material_blend?material_blend:model.effect?model.effect->submission.blend:0;
            uint32_t depth_token=0;
            if(model.effect){const bool no_write=mesh.programs->policy->disable_depth_write;const int32_t d=model.effect->submission.arg_depth;
                const bool pair=mesh.material_class && (*mesh.material_class&0x10D) && !(model.effect->submission.arg_flags>>17&1);
                depth_token=pair?((d!=0 && !mesh.programs->policy->transparent)?0x1E0u:0x1E8u):(d==1 || (d!=0 && !no_write))?0x1E0u:0x1E8u;}   // I-0140 tranche 6: +0x111 takes +0x868
            const bool effect_blended=model.effect && blend_token && ds3native::NativeEffectBankBlend(blend_token,effect_blend);
            const bool effect_depth_set=model.effect && depth_token && ds3native::NativeEffectBankDepth(depth_token,effect_depth);
            if(model.effect){static LONG64 bank_rows{},fallbacks{};const auto n=InterlockedIncrement64(&bank_rows);if(!effect_blended || !effect_depth_set)InterlockedIncrement64(&fallbacks);
                if(n==1 || n%3000==0){char row[200]{};sprintf_s(row,"{\"event\":\"native_effect_model_states\",\"rows\":%lld,\"fallbacks\":%lld,\"pass\":%d,\"blend\":\"%X\",\"depth\":\"%X\"}\n",
                    static_cast<long long>(n),static_cast<long long>(InterlockedAdd64(&fallbacks,0)),model.effect->submission.pass,blend_token,depth_token);Log(row);}}
            // I-0113 tranche 10b: an ordinary row's blend by the original's rule (NativeOrdinaryForwardBlend: the group's kind, else the
            // material's blend mode with the group's +0x85 row); before it a material whose mode was neither 0 nor 2 was not drawn.
            // P9 (PHANTOMS-SIGNS-NATIVE §5): rows 0x21/0x22 are opaque and write depth (LESS_EQUAL, write ALL). I-0113 tranche 10d
            // (gap 19, a dying character vanishing at once): the generic row 0x29 of a category-0x29 model (fading) takes the same
            // rule, as 5A120 → 1A86C0 → 140200840 give it to every row (the fade sets +0x85 to 1: at least alpha, by the table's
            // row 1); before it a fading material whose mode was neither 0 nor 2 was skipped, the rest blended by alpha (explicit
            // policy, kept where the rule does not resolve).
            const bool phantom_opaque=prepared.engine_pass==0x21 || prepared.engine_pass==0x22;
            const bool fading=prepared.engine_pass==0x29 && model.ordinary_category && *model.ordinary_category==0x29;
            D3D12_BLEND_DESC ordinary_blend{};
            const bool ordinary_blended=!model.effect && !water_draw && !phantom_opaque && model.engine_bytes && model.engine_bytes->group_read &&
                NativeOrdinaryForwardBlend(model.engine_bytes->group[0x84],model.engine_bytes->group[0x85],blend_mode,ordinary_blend);
            // I-0113 tranche 10d: a draw's depth pre-pass (rows_of: a row of engine pass 0x29 with a Dpt recipe, before its colour rows).
            const bool depth_prepass=prepared.engine_pass==0x29 && (prepared.recipe==NativeRecipe(2,0) || prepared.recipe==NativeRecipe(2,1));
            if(blend_mode!=0 && blend_mode!=2 && !effect_blended && !ordinary_blended && !depth_prepass){static LONG64 blend_samples{};
                if(InterlockedIncrement64(&blend_samples)<=12){char row[160]{};sprintf_s(row,"{\"event\":\"native_forward_blend_unsupported\",\"blend_mode\":%u}\n",blend_mode);Log(row);}
                ++missing_constants;continue;}
            kprofile::SwitchIfActive(kprofile::PrepForwardLight);const bool forward_lit=bool(model.forward_light);   // cP7: built at collect
            kprofile::SwitchIfActive(kprofile::PrepRowSetup);
            if(!forward_lit){static LONG64 light_samples{};
                if(InterlockedIncrement64(&light_samples)<=12)Log("{\"event\":\"native_forward_light_unavailable\"}\n");
                ++missing_constants;continue;}
            input.render_target_count=1;input.render_target_formats={};input.render_target_formats[0]=SceneRenderer::ForwardTargetFormat();
            input.shape.envelope=ModelPassEnvelope::Forward1;
            input.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;input.depth_stencil.StencilEnable=FALSE;
            // I-0113 tranche 10d: an ordinary model's forward row takes Begin's depth (140140680:248-268)
            // with args+0x188 = the draw item's +0x1AA, the emit descriptor's byte +0x0C, which CFDE70, D079C0 and D3DA20 (every
            // ordinary emitter: parts, draw entities, map pieces and objects) leave at 0xFF (0x10000FF at +0x0C, OBSERVED on the
            // player's parts, reference_part_row_state): written for a material class with a 0x10D bit (the forward pair +0x860; the
            // MTD's +0x111 is clear for classes 1, 4 and 0x100; a Decal of class 8 reaching a fading 0x29 row or a phantom row may carry it: then
            // +0x868, not written, I-0140 tranche 6's final review L1), else unless the material disables depth writes
            // (the MTD's +0x81, INFERRED to be g_GX_bDisableDepthWrite, as for the E03 rows). The pair's stencil is not modelled.
            // Phantom rows 0x21/0x22 (P9) write depth. I-0113 tranche 15: the rows now drawn in 57A20's order (SortNativeForwardRows),
            // every 0x1F and 0x29 row takes the rule (before it a category-2 0x29 row wrote no depth, an explicit policy while the
            // rows were drawn in model order: the step review's M2).
            const bool depth_rule_rows=prepared.engine_pass==0x1F || prepared.engine_pass==0x29;
            const bool begin_written=!water_draw && !model.effect && depth_rule_rows &&
                ((mesh.material_class && (*mesh.material_class&0x10Du))?!mesh.programs->policy->transparent:!mesh.programs->policy->disable_depth_write);
            if(phantom_opaque || begin_written)input.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;
            if(!phantom_opaque && (blend_mode==2 || fading)){auto& target=input.blend.RenderTarget[0];target.BlendEnable=TRUE;
                target.SrcBlend=D3D12_BLEND_SRC_ALPHA;target.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;
                target.SrcBlendAlpha=D3D12_BLEND_ONE;target.DestBlendAlpha=D3D12_BLEND_INV_SRC_ALPHA;}
            // I-0140 (gap 23): a row dissolving in the world's view also draws into the TAA's fade mask (model_draw.h
            // fade_mask), where its pixels then keep no history and their own sample (shaders/taa.hlsl): the dissolve the original shows
            // without anti-aliasing. Its step review's L2: only where the TAA with its reactive weighting will read the mask (no pipeline
            // made for DLSS's frames). Its final review's H1 and M1: a category-0x29 row is not always a dissolve: not an effect's model
            // particle (category 0x29 in pass 0x29 for its whole life: the bonfire's coiled sword), not a phantom (0x29 for its whole
            // presence); the pipeline is made for the others, and a row marks the mask only on a frame its model's fade value moved
            // (NativeFadeMoving, fade_mark: a dissolve, not a translucency that lasts). Any model dissolving so marks it (an illusory
            // wall's fade too), not only a character's.
            input.fade_mask=fading && !model.effect && !model.phantom && !offscreen && !depth_prepass && scene->taa && NativeTaaReactive();   // its colour rows mark it (a pre-pass may have no pixel programme)
            if(input.fade_mask && model.engine_bytes && model.engine_bytes->group_read){float value{};std::memcpy(&value,model.engine_bytes->group.data()+0xDC,4);
                fade_mark=NativeFadeMoving(model.entity,model.model,value,presentation_serial);}
            // WATER-TESS-WAVE1-READY 1.2 (OBSERVED, WATER-NATIVE 1): WatDpt (0x24) blends SRC_ALPHA / INV_SRC_ALPHA with alpha ZERO / ZERO
            // (its alpha 0 is the refraction mask the surface tests in the frame copy) and writes no depth; the surface (0x27) the
            // same colour blend with alpha ONE / ZERO and writes depth; both keep the face set's raster state (OBSERVED cull back, front counter-clockwise).
            if(water_draw){auto& target=input.blend.RenderTarget[0];target.BlendEnable=TRUE;
                target.SrcBlend=D3D12_BLEND_SRC_ALPHA;target.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;target.BlendOp=D3D12_BLEND_OP_ADD;
                target.SrcBlendAlpha=prepared.engine_pass==0x24?D3D12_BLEND_ZERO:D3D12_BLEND_ONE;target.DestBlendAlpha=D3D12_BLEND_ZERO;target.BlendOpAlpha=D3D12_BLEND_OP_ADD;
                input.depth_stencil.DepthWriteMask=prepared.engine_pass==0x27?D3D12_DEPTH_WRITE_MASK_ALL:D3D12_DEPTH_WRITE_MASK_ZERO;}
            if(ordinary_blended){
                // Witness (light-observe.flag, the first 32 programme sets, each by its first kind and row): a row whose rule-given colour
                // blend differs from the former policy (mode 0 unblended, mode 2 SRC_ALPHA / INV_SRC_ALPHA ADD; the alpha channel and the
                // write mask not compared; other modes skipped); passing the blend gate, not yet drawn.
                const auto& rt=ordinary_blend.RenderTarget[0];
                const bool former=(blend_mode==0 && !rt.BlendEnable) || (blend_mode==2 && rt.BlendEnable && rt.SrcBlend==D3D12_BLEND_SRC_ALPHA &&
                    rt.DestBlend==D3D12_BLEND_INV_SRC_ALPHA && rt.BlendOp==D3D12_BLEND_OP_ADD);
                static const bool rule_witness=ModuleFlag(L"light-observe.flag");static std::atomic<bool> changed_full{};
                if(rule_witness && !former && !changed_full.load(std::memory_order_relaxed)){static std::mutex changed_lock;static std::set<const void*> changed;bool fresh{};
                    {const std::lock_guard hold(changed_lock);fresh=changed.size()<32 && changed.insert(mesh.programs.get()).second;if(changed.size()>=32)changed_full=true;}
                    if(fresh){char row[320]{};sprintf_s(row,"{\"event\":\"native_forward_blend_rule\",\"mode\":%u,\"kind\":%u,\"row\":%u,\"pass\":%u,\"blend\":[%d,%d,%d,%d],\"family\":\"%.160s\"}\n",
                        blend_mode,model.engine_bytes->group[0x84],model.engine_bytes->group[0x85],prepared.engine_pass,rt.BlendEnable,rt.SrcBlend,rt.DestBlend,rt.BlendOp,
                        NativeFamilyKey(*mesh.programs).c_str());Log(row);}}
                input.blend=ordinary_blend;}
            if(effect_blended)input.blend=effect_blend;   // I-0105: the effect context's states (above)
            if(effect_depth_set){input.depth_stencil=effect_depth;input.depth_stencil.StencilEnable=FALSE;}
            // The pre-pass (5A120's preparation 1: args+0x200 bit 19 picks the bank entry 0x32, no colour written, OBSERVED as write
            // mask 0 on the hair in i113-t10d-hair-ref, whose group kind +0x84 is 0xFF; explicit policy for a group of another kind,
            // where 1A86C0's kind blend stays bound and 140200840 does not run: not seen). Its depth is Begin's (begin_written above).
            if(depth_prepass){input.blend.IndependentBlendEnable=FALSE;input.blend.AlphaToCoverageEnable=FALSE;
                for(auto& target:input.blend.RenderTarget){target.BlendEnable=FALSE;target.RenderTargetWriteMask=0;}}
            // W6 GL-6 (GLOW-WAVE1-READY 5.6): a forward pixel programme that declares SV_Target1 (o1 = the emissive glow) writes
            // GB5 as its second target, which the glow's extraction reads; RT1 takes RT0's blend (the original's
            // IndependentBlendEnable is 0). One-output programmes stay Forward1 (the same image as the original's write mask 0).
            // The menu views keep Forward1 (no glow there).
            const auto& pixel_interface=mesh.programs->interfaces[prepared.recipe][1];
            if(!offscreen && pixel_interface && WritesSecondTarget(*pixel_interface)){
                input.render_target_count=2;input.render_target_formats[1]=SceneRenderer::ModelTargetFormats()[5];
                input.shape.envelope=ModelPassEnvelope::Forward2;
                if(!input.blend.IndependentBlendEnable)input.blend.RenderTarget[1]=input.blend.RenderTarget[0];}   // an independent bank entry keeps its RT1
            {static volatile LONG64 forward_gb5_rows{};static volatile LONG64 forward_gb5_second{};const auto n=InterlockedIncrement64(&forward_gb5_rows);
                const auto g=input.shape.envelope==ModelPassEnvelope::Forward2?InterlockedIncrement64(&forward_gb5_second):InterlockedAdd64(&forward_gb5_second,0);
                if(n==1 || n%20000==0){char row[192]{};sprintf_s(row,"{\"event\":\"native_forward_gb5\",\"forward_rows\":%lld,\"second_target_rows\":%lld}\n",
                    static_cast<long long>(n),static_cast<long long>(g));Log(row);}}
        }
        // 1186 velocity row 0x34 (group 0x33 targets): one velocity target, opaque, no stencil. The original tests the rows against a
        // half-size copy of the scene depth (pipeline+0x3980, Reference1186) without writing it; natively (I-0136) they draw into a depth
        // of their own (written, nearest wins) and the renderer's occlusion pass compares it with the full-resolution scene depth,
        // within the rows' rounding (the Velo VS projects with the absolute view-projection where the G-buffer VS used the
        // camera-relative one); velo-depth-legacy.flag keeps the former test against the scene depth with a small bias.
        if(velocity_draw){
            input.render_target_count=1;input.render_target_formats={};input.render_target_formats[0]=SceneRenderer::VelocityTargetFormat();
            input.shape.envelope=ModelPassEnvelope::Velocity1;
            input.depth_stencil.StencilEnable=FALSE;
            // I-0136: by default the rows draw into a depth of their own (nearest wins, written, no bias) and the renderer's occlusion pass
            // compares them with the scene depth; velo-depth-legacy.flag keeps the former test against the scene depth, unwritten, with
            // a small bias toward the camera, which was too small at Firelink's coordinates (flat triangles failed whole).
            if(!VeloDepthLegacy()){input.depth_stencil.DepthEnable=TRUE;input.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;
                input.depth_stencil.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;input.rasterizer.DepthBias=0;input.rasterizer.SlopeScaledDepthBias=0.0f;}
            else{input.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;input.rasterizer.DepthBias=-64;input.rasterizer.SlopeScaledDepthBias=-2.0f;}
            // I-0134 (the cloth's own velocity): a mesh drawing its simulated copy with the material's cloth on (VC_isEnableCloth = 1,
            // the admission's 1184 rule) takes the native cloth Velo programme, whose previous position is the previous copy's
            // (slot 2); the game's programme placed the current copy twice, so a cape moving against its frame had no motion.
            // Before a second upload, in a frame without an upload (no previous copy) or for a programme that is not a cloth Velo one,
            // the game's programme stays.
            static const bool no_cloth_velocity=ModuleFlag(L"no-cloth-velocity.flag");
            // The produced VC_isEnableCloth word itself (cbMatDynParam +0x80, the admission's 1108/1184 block), not its rule again (its
            // second final review's L3).
            const auto cloth_word=[&]{uint32_t word{};if(mesh.dynamic_bytes.size()>=0x84)std::memcpy(&word,mesh.dynamic_bytes.data()+0x80,4);return word;};
            if(!no_cloth_velocity && mesh.cloth_stream && mesh.cloth_previous && cloth_word()==1u)
                if(const auto set=NativeClothVelocityProgramSetOf(*mesh.programs,prepared.recipe,*input.program_set)){
                    // cloth-velocity-observe.flag: the first 16 rows' inputs (and the admissions' upload ages, 09).
                    static const bool observe=ModuleFlag(L"cloth-velocity-observe.flag");
                    auto geometry=input.geometry;
                    if(BindModelPreviousStream(geometry,mesh.cloth_previous)){
                        input.program_set=set;input.geometry=std::move(geometry);
                        static LONG64 cloth_velocity_rows{};const auto n=InterlockedIncrement64(&cloth_velocity_rows);
                        if(observe && n<=16){uint32_t enable=UINT32_MAX;
                            if(mesh.dynamic_bytes.size()>=0x84)std::memcpy(&enable,mesh.dynamic_bytes.data()+0x80,4);
                            std::string layout;for(const auto& e:set->elements){char part[80]{};
                                sprintf_s(part,"%s%.12s%u:%d@%u/%u",layout.empty()?"":",",e.semantic.c_str(),e.semantic_index,int(e.format),e.input_slot,e.byte_offset);layout+=part;}
                            char row[760]{};sprintf_s(row,"{\"event\":\"native_cloth_velocity_row\",\"n\":%lld,\"enable\":%u,\"recipe\":%zu,\"stride\":%u,\"second\":%s,\"pixel_bytes\":%zu,\"layout\":\"%.560s\"}\n",
                                static_cast<long long>(n),enable,size_t(prepared.recipe),input.geometry.vertices.StrideInBytes,input.geometry.vertices2.BufferLocation?"true":"false",
                                set->pixel.bytecode.size(),layout.c_str());Log(row);}
                        if(n<=4 || n%3000==0){char row[160]{};
                            sprintf_s(row,"{\"event\":\"native_cloth_velocity_rows\",\"rows\":%lld}\n",static_cast<long long>(n));Log(row);}}}
        }
        bool complete=true;
        // 1091 witness: the resource on which a character part draw stops (last visited).
        const char* failed_resource{};uint32_t failed_register{},failed_input{},failed_stage{};std::string srv_names; // 1094: character-part SRV sources
        // m45 frame cost (Native1841: the names were formatted for every rebuilt character row): only for a row one of the two
        // witnesses below will write (the 1177 menu forward rows, the 1094 part rows: their first ones, then every 600th).
        static LONG64 hair_srv_samples{},asm_srv_samples{};
        const bool named_witness=!offscreen && hide_fragment.size()>1 && hide_fragment[0]==L'!' && chair_hunt::Matches(hide_fragment,model.model);
        const bool srv_witness=named_witness || (offscreen && forward_draw && InterlockedAdd64(&hair_srv_samples,0)<12) ||   // review L5: read atomically (chunks)
            (model.asm_part>=0 && model.asm_part<2 && [&]{const auto n=InterlockedAdd64(&asm_srv_samples,0);return n<24 || (n+1)%600==0;}());
        LARGE_INTEGER resources_start{};QueryPerformanceCounter(&resources_start);
        for(size_t stage=0;stage<2 && complete;++stage){
            const auto& shader=mesh.programs->interfaces[prepared.recipe][stage];
            const auto& reads=mesh.programs->ReadsOf(prepared.recipe,stage);
            if(!shader){++missing_interfaces;complete=false;break;}
            const auto shader_stage=stage?ds3content::ShaderStage::Pixel:ds3content::ShaderStage::Vertex;
            for(const auto& resource:shader->resources){
                failed_resource=resource.name.c_str();failed_register=resource.shader_register;failed_input=resource.input_type;failed_stage=static_cast<uint32_t>(stage);
                if(resource.input_type==0){
                    const auto slot=resource.shader_register;
                    ds3content::ShaderRegisterBlock block;
                    // PREPARE-REDUCTION cP5: an immutable shared block bound as is (no copy); the draw uploads it once per chunk life.
                    std::shared_ptr<const ds3content::ShaderRegisterBlock> shared_block;
                    // cP0: the phase of this register, in the order of the chain below.
                    kprofile::SwitchIfActive(slot==3 || (slot==8 && !(stage && forward_draw)) || slot==4 || (!stage && slot==0 && !velocity_draw)?kprofile::PrepStored:
                        slot==2?kprofile::PrepMatDyn:slot==7?kprofile::PrepModelParams:
                        (slot==8 || slot==9 || (!stage && slot==0) || (stage && forward_draw && (slot==5 || slot==6)))?kprofile::PrepFrameBlocks:
                        (!stage && (slot==5 || slot==12 || (velocity_draw && slot==6)))?kprofile::PrepPalettes:kprofile::PrepOtherBlocks);
                    if(NativeModelFrameRootOf(static_cast<uint32_t>(stage),slot)<FrameRootCount)
                        shared_block=frame_block_of(mesh,prepared,stage,slot,velocity_draw,forward_draw);   // C4-1 frame registers (C4-2: as for a kept draw)
                    else if(slot==3){if(const auto& stored=mesh.numeric_blocks[prepared.recipe][stage])shared_block=stored;}
                    else if(slot==8 && !(stage && forward_draw)){if(const auto& stored=NativeMeshSceneBlock(*scene,mesh,prepared.recipe,stage))shared_block=stored;}
                    else if(slot==8){
                        // 1116 forward PS cbSceneParam: the camera words, plus the clustered
                        // light-grid words (I-0107, NativeForwardGridWords).
                        const auto produce=[&]{auto bytes=scene->camera_constants.bytes;auto written=scene->camera_constants.written;
                            NativeForwardGridWords(*scene,offscreen,bytes,written);   // I-0107
                            const auto ranges=NativeShaderWordRanges(bytes,written);
                            return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbSceneParam");};
                        shared_block=share({mesh.programs.get(),nullptr,prepared.recipe,(1u<<16)|(stage<<8)|slot},produce);
                    }
                    else if(slot==2){
                        // PREPARE-REDUCTION cP5: the block is a pure function of the mesh's dynamic bytes and their coverage
                        // (made at collect), the programme recipe and stage, and the inactive-parameter inputs; kept per Programs
                        // by the whole key (64 entries, oldest dropped).
                        // I-0140 tranche 6: the Decal G-buffer rows (class 8) too, as when they kept the class 1 or 4 (the step review's M2).
                        const bool inactive_applies=stage && scene->pixel_policy==ds3content::NativeModelPixelPolicy::Default &&
                            mesh.material_class && (*mesh.material_class==1 || *mesh.material_class==4 || *mesh.material_class==8);
                        const int8_t wet_key=mesh.wet_activation?(*mesh.wet_activation?1:0):-1;
                        const auto same_matdyn=[&](const NativeModelLayout::Programs::MatDynEntry& entry){
                            return entry.recipe==prepared.recipe && entry.stage==stage && entry.inactive==inactive_applies && entry.wet==wet_key &&
                                entry.bytes==mesh.dynamic_bytes && entry.written==mesh.dynamic_written;};
                        std::shared_ptr<const ds3content::ShaderRegisterBlock> kept_matdyn;
                        {const std::lock_guard hold(mesh.programs->matdyn_lock);
                            for(const auto& entry:mesh.programs->matdyn_blocks)if(same_matdyn(entry)){kept_matdyn=entry.block;break;}}
                        shared_block=kept_matdyn;
                        if(!shared_block){
                        auto ranges=NativeShaderWordRanges(mesh.dynamic_bytes,mesh.dynamic_written);
                        std::vector<ds3content::ModelParameterWrite> inactive;
                        if(inactive_applies){
                            const std::optional<bool> damage_enabled=mesh.programs->damage_parameters?
                                std::optional<bool>{mesh.programs->damage_parameters->enabled}:std::nullopt;
                            inactive=ds3content::ProduceOrdinaryInactiveModelParameters(*shader,damage_enabled,mesh.wet_activation);
                            for(const auto& write:inactive)ranges.push_back({write.byte_offset,
                                {reinterpret_cast<const uint8_t*>(write.words.data()),write.bytes}});
                        }
                        block=PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbMatDynParam");
                        if(block.status!=ds3content::ShaderConstantStatus::Ready){
                            static LONG64 matdyn_samples{},asm_matdyn_samples{};
                            // 1092: also the first 12 character-part failures (body materials).
                            if(InterlockedIncrement64(&matdyn_samples)<=16 || (model.asm_part>=0 && InterlockedIncrement64(&asm_matdyn_samples)<=12)){
                                char matdyn_missing[512]{};size_t matdyn_cursor{};uint32_t matdyn_count{};bool matdyn_offsets_complete=true;
                                const auto matdyn_declaration=std::find_if(shader->constants.begin(),shader->constants.end(),
                                    [&](const auto& candidate){return candidate.name==resource.name;});
                                if(reads.ok && matdyn_declaration!=shader->constants.end() && matdyn_declaration->declared_bytes<=65536){
                                    const auto matdyn_reads=ds3reads::FloatMask(reads,slot,matdyn_declaration->declared_bytes,static_cast<bool*>(nullptr));
                                    for(size_t matdyn_word=0;matdyn_word<matdyn_reads.size();++matdyn_word)
                                        if(matdyn_reads[matdyn_word] && std::none_of(ranges.begin(),ranges.end(),[&](const auto& range){
                                            return matdyn_word*4>=range.byte_offset && matdyn_word*4+4<=uint64_t(range.byte_offset)+range.bytes.size();})){
                                            ++matdyn_count;
                                            const auto covering=std::find_if(matdyn_declaration->variables.begin(),matdyn_declaration->variables.end(),[&](const auto& variable){
                                                return matdyn_word*4>=variable.byte_offset && matdyn_word*4<uint64_t(variable.byte_offset)+variable.bytes;});
                                            if(matdyn_cursor+64<sizeof(matdyn_missing))matdyn_cursor+=sprintf_s(matdyn_missing+matdyn_cursor,
                                                sizeof(matdyn_missing)-matdyn_cursor,"%s\"%zu:%.40s\"",matdyn_cursor?",":"",matdyn_word*4,covering!=matdyn_declaration->variables.end()?covering->name.c_str():"?");
                                            else matdyn_offsets_complete=false;
                                        }
                                }
                                char matdyn_row[1024]{};sprintf_s(matdyn_row,
                                    "{\"event\":\"native_selected_matdyn_unavailable\",\"asm_part\":%d,\"view_serial\":%llu,\"mesh_ordinal\":%u,\"recipe\":%u,\"stage\":%zu,\"status\":%u,\"read_analysis\":%s,\"missing_word_count\":%u,\"missing_byte_offsets\":[%s],\"offsets_complete\":%s,\"animations_available\":%s,\"animation_rows\":%zu,\"generated_owner\":%s,\"wet_activation\":%u,\"wet_material\":%s,\"wet_draw\":%s,\"damage_owner\":%s,\"generated_offset\":%u,\"generated_words\":%zu}\n",
                                    model.asm_part,scene->serial,mesh.current.ordinal,prepared.recipe,stage,static_cast<unsigned>(block.status),reads.ok?"true":"false",
                                    matdyn_count,matdyn_missing,matdyn_offsets_complete?"true":"false",mesh.parameter_animations_available?"true":"false",mesh.parameter_animation_rows,
                                    mesh.programs->generated_parameters?"true":"false",mesh.wet_activation?(*mesh.wet_activation?1u:0u):2u,
                                    mesh.wet_parameters?"true":"false",model.wet_draw_inputs?"true":"false",mesh.programs->damage_parameters?"true":"false",
                                    mesh.programs->generated_parameters?mesh.programs->generated_parameters->byte_offset:0u,
                                    mesh.programs->generated_parameters?mesh.programs->generated_parameters->words.size():size_t(0));
                                Log(matdyn_row);
                            }
                        }
                        {auto made=std::make_shared<const ds3content::ShaderRegisterBlock>(std::move(block));
                            const std::lock_guard hold(mesh.programs->matdyn_lock);auto& blocks=mesh.programs->matdyn_blocks;
                            std::erase_if(blocks,same_matdyn);if(blocks.size()>=64)blocks.erase(blocks.begin());
                            blocks.push_back({prepared.recipe,static_cast<uint32_t>(stage),inactive_applies,wet_key,mesh.dynamic_bytes,mesh.dynamic_written,made});
                            shared_block=std::move(made);}
                        }
                    }
                    else if(stage && slot==0 && resource.name=="cbTexHdrScaleParam"){
                        // Old host producer (I-0063): the optional float metadata of the
                        // TPF entry of the material's lightmap g_DOLTexture1 (t26), at
                        // FC_TexHdrScaleParam.
                        const auto lightmap=std::find_if(mesh.texture_bindings.begin(),mesh.texture_bindings.end(),
                            [](const auto& row){return row.input.declaration.slot==26 && row.texture;});
                        const auto produce=[&]{ds3content::ShaderRegisterBlock produced;
                            const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),
                                [&](const auto& candidate){return candidate.name==resource.name;});
                            // m30 (Native1757: 2,585 of 2,745 rows dropped for this block): a lightmapped material whose lightmap carries
                            // no float block, or whose lightmap is not delivered, takes (1, 1, 1, 1), what the game uploads then (old host
                            // rule, I-0063: m39 m000210, object lightmaps "_o<id>_gi...").
                            if(declaration!=shader->constants.end() && declaration->declared_bytes<=4096){
                                const auto variable=std::find_if(declaration->variables.begin(),declaration->variables.end(),
                                    [](const auto& row){return row.name=="FC_TexHdrScaleParam";});
                                if(variable!=declaration->variables.end()){
                                    std::vector<float> floats;
                                    // Lightmap floors: the draw's own lightmap's authored constant, as 5B110 binds it (else the TPF metadata).
                                    if(mesh.lightmap_constant)floats.assign(mesh.lightmap_constant->begin(),mesh.lightmap_constant->end());
                                    else if(lightmap!=mesh.texture_bindings.end() && lightmap->texture->content && lightmap->texture->content->float_metadata)
                                        for(const auto word:lightmap->texture->content->float_metadata->words){float value{};std::memcpy(&value,&word,4);floats.push_back(value);}
                                    else floats={1.f,1.f,1.f,1.f};
                                    // Lightmap floors (WAVE1-LAST-EFFECTS-STATIC-PREP 1.5): which blocks carry the lightmap's metadata.
                                    {static LONG64 hdr_scale_counts[4]{};auto& with=hdr_scale_counts[0];auto& without=hdr_scale_counts[1];auto& unbound=hdr_scale_counts[2];auto& rows=hdr_scale_counts[3];
                                        const bool has=lightmap!=mesh.texture_bindings.end(),meta=has && lightmap->texture->content && lightmap->texture->content->float_metadata;
                                        InterlockedIncrement64(meta?&with:has?&without:&unbound);
                                        if(const auto n=InterlockedIncrement64(&rows);n<=6 || n%3000==0){char row[320]{};
                                            sprintf_s(row,"{\"event\":\"native_lightmap_hdr_scale\",\"blocks\":%lld,\"metadata\":%lld,\"no_metadata\":%lld,\"no_lightmap\":%lld,\"floats\":[%g,%g,%g,%g]}\n",
                                                n,with,without,unbound,floats.size()>0?floats[0]:0.f,floats.size()>1?floats[1]:0.f,floats.size()>2?floats[2]:0.f,floats.size()>3?floats[3]:0.f);Log(row);}}
                                    std::vector<uint8_t> bytes(declaration->declared_bytes),written(declaration->declared_bytes/4);
                                    ds3prod::ProduceTexHdrScale(floats,variable->byte_offset,bytes.data(),declaration->declared_bytes,written.data());
                                    const auto ranges=NativeShaderWordRanges(bytes,written);
                                    produced=PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbTexHdrScaleParam");
                                }
                            }
                            return produced;};
                        // cP5: per programme recipe and stage and lightmap record (its content is immutable), call-local.
                        // A lightmap whose content is not yet there is produced per call (its later metadata must not meet a kept 1, 1, 1, 1).
                        // Without a lightmap binding the block is (1, 1, 1, 1), shared per programme recipe and stage (review).
                        if(lightmap!=mesh.texture_bindings.end() && !lightmap->texture->content)block=produce();
                        else shared_block=share({mesh.programs.get(),lightmap!=mesh.texture_bindings.end()?lightmap->texture.get():nullptr,prepared.recipe,(6u<<16)|(stage<<8)|slot},produce);
                    }
                    else if(!stage && slot==7 && resource.name=="cbvModelParam"){
                        // VS cbvModelParam, 5B3D0's second block (old host ProduceModelParam,
                        // I-0043/I-0063): +00 current and +30 previous 3x4 world rows
                        // (a static map piece: its placement twice), +60 group+50,
                        // +4B0 group+60, +80 the animation block [anim+210] when
                        // anim+620 (else zero, F1 below), +4C0 zero unless anim+630; +70 and
                        // +490 zero where the programme does not use them (F1).
                        const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),
                            [&](const auto& candidate){return candidate.name==resource.name;});
                        if(declaration!=shader->constants.end() && declaration->declared_bytes==ds3prod::model_param_bytes){
                            std::vector<uint8_t> bytes(ds3prod::model_param_bytes),written(ds3prod::model_param_bytes/4);
                            const auto put=[&](uint32_t offset,const void* source,uint32_t count){std::memcpy(bytes.data()+offset,source,count);
                                for(uint32_t i=0;i<count;i+=4)written[(offset+i)/4]=1;};
                            put(0x00,model.placement.data(),48);put(0x30,model.placement.data(),48);
                            // 1185: a cloth draw with its simulated copy bound uses the cloth instance frame (VC_World only).
                            if(mesh.cloth_stream && model.cloth_world){put(0x00,model.cloth_world->data(),48);put(0x30,model.cloth_world->data()+12,48);}
                            const auto& engine=*model.engine_bytes;   // cP6: the collect's copy of the engine bytes
                            if(engine.group_read)put(0x60,engine.group.data()+0x50,16);
                            if(engine.group_read)put(0x4B0,engine.group.data()+0x60,16);
                            // F1 (I-0068 FOLIAGE-NATIVE §8): the Grass vertex programmes index this buffer by register, so the
                            // read analysis requires every word. +70 (VFC_PomLightDir) and +490..+4AF (VC_InstanceId, its pad,
                            // the unused clip plane) are unused by them and take zero; +80..+48F (VC_ForceParam) is zero when the
                            // animation block is not set (wind counts 0: rest pose, the constructor's initial state, 140D3C4D0);
                            // +4C0..+4EF zero when [anim+630] is clear or the animation object is absent.
                            const auto block_flag=engine.block_flag,matrix_flag=engine.matrix_flag;
                            const bool block_read=model.animation && engine.block_flag_read;
                            const bool matrix_read=model.animation && engine.matrix_flag_read;
                            if(model.animation && (!block_read || !matrix_read)){static LONG64 unreadable{};if(InterlockedIncrement64(&unreadable)<=8)
                                Log("{\"event\":\"native_vmodel_animation_unreadable\"}\n");}
                            // Only where the compiler marks every variable of the range unused (RDEF D3D_SVF_USED clear): POM programmes
                            // read VFC_PomLightDir (+70), whose producer (context +1290) is still absent.
                            const auto unused=[&](uint32_t begin,uint32_t end){return std::none_of(declaration->variables.begin(),declaration->variables.end(),
                                [&](const auto& v){return v.byte_offset<end && v.byte_offset+v.bytes>begin && (v.flags&2u);});};
                            if(unused(0x70,0x80)){const std::array<uint8_t,16> zero{};put(0x70,zero.data(),16);}
                            {std::array<uint8_t,0x410> animation_block{};
                                if(block_read && block_flag && engine.block_read)animation_block=engine.animation_block;
                                put(0x80,animation_block.data(),0x410);}
                            if(unused(0x490,0x4B0)){const std::array<uint8_t,32> zero{};put(0x490,zero.data(),32);}
                            // An unreadable +630 with an animation object leaves +4C0 unproduced (refused, as before F1).
                            if(!model.animation || (matrix_read && !matrix_flag)){const std::array<uint8_t,48> zero{};put(0x4C0,zero.data(),48);}
                            const auto ranges=NativeShaderWordRanges(bytes,written);
                            block=PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbvModelParam");
                            if(block.status!=ds3content::ShaderConstantStatus::Ready && reads.ok){
                                static LONG64 vmodel_samples{};
                                if(InterlockedIncrement64(&vmodel_samples)<=6){std::string missing;
                                    const auto mask=ds3reads::FloatMask(reads,slot,declaration->declared_bytes,static_cast<bool*>(nullptr));
                                    for(size_t word=0;word<mask.size() && missing.size()<700;++word)if(mask[word] && !written[word]){
                                        const auto covering=std::find_if(declaration->variables.begin(),declaration->variables.end(),[&](const auto& variable){
                                            return word*4>=variable.byte_offset && word*4<uint64_t(variable.byte_offset)+variable.bytes;});
                                        char item[96]{};sprintf_s(item,"%s\"%zu:%.40s\"",missing.empty()?"":",",word*4,covering!=declaration->variables.end()?covering->name.c_str():"?");missing+=item;}
                                    char row[1024]{};sprintf_s(row,"{\"event\":\"native_vmodel_param_unproduced\",\"words\":[%.900s]}\n",missing.c_str());Log(row);}
                            }
                        }
                    }
                    else if(stage && slot==7 && resource.name=="cbModelParam"){
                        // PS cbModelParam: 1A86C0 assembles it from the draw state,
                        // whose +240 is the selected 400-byte group (1A8580) and
                        // +200 bit20 the fade dither (old host ProduceModelParamPs,
                        // I-0063/I-0064, with the .data default row 4929B20).
                        std::array<uint8_t,ds3prod::draw_state_bytes> state{};std::array<uint8_t,16> default_row{};
                        const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),
                            [&](const auto& candidate){return candidate.name==resource.name;});
                        if(declaration!=shader->constants.end() && declaration->declared_bytes<=4096 && !(declaration->declared_bytes%4) &&
                           model.engine_bytes->group_read && scene->default_param_row){   // cP6: the collect's copies
                            std::memcpy(state.data()+0x240,model.engine_bytes->group.data(),0x190);default_row=*scene->default_param_row;
                            if(model.draw_colour)std::memcpy(state.data()+0x280,model.draw_colour->data(),16);   // 1169: D3DA20's per-item copy
                            // P4: CFDE70's phantom changes to its stack copy of the group (state = group + 0x240): +0x87 = 1,
                            // +0x84 = kind, mulCol.w (group +0xDC) times the row fade.
                            if(model.phantom){state[0x240+0x87]=1;state[0x240+0x84]=model.phantom_kind;float alpha{};
                                std::memcpy(&alpha,state.data()+0x240+0xDC,4);alpha*=model.phantom_fade;std::memcpy(state.data()+0x240+0xDC,&alpha,4);}
                            const auto& fade=model.geometry_fades.intents[prepared.fade];
                            if(fade.fade_enabled && scene->category_scalar && fade.opacity<*scene->category_scalar){
                                uint32_t flags{};std::memcpy(&flags,state.data()+0x200,4);flags|=1u<<20;std::memcpy(state.data()+0x200,&flags,4);}
                            // 1186: 5A120's Velo preparation (case 5) writes state +0x310 = FC_DifColMul (the velocity scale).
                            if(velocity_draw){std::memcpy(state.data()+0x310,scene->velocity->scale.data(),16);row_velocity_scale=scene->velocity->scale;}
                            std::vector<uint8_t> bytes(declaration->declared_bytes),written(declaration->declared_bytes/4);
                            ds3prod::ProduceModelParamPs(state.data(),bytes.data(),declaration->declared_bytes,written.data(),default_row.data());
                            ProduceNativeSnowRate(*scene,state.data()+0x240,bytes.data(),declaration->declared_bytes,written.data());   // I-0105
                            if(current_snow && declaration->declared_bytes>=0x44)row_snow=current_snow;   // B5: FC_SnowRate (+0x40) written
                            // 1115: with both parallax-corrected cubemap boxes disabled (state +426/+427
                            // clear: enables 0 at +1A0/+1A4) the shader never uses the box words; they
                            // keep the creation zero (explicit open policy: no box inputs natively yet).
                            if(!state[0x426] && !state[0x427])for(uint32_t at=0x80;at<0x1A0 && at+4<=declaration->declared_bytes;at+=4)written[at/4]=1;
                            const auto ranges=NativeShaderWordRanges(bytes,written);
                            block=PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbModelParam");
                        }
                    }
                    else if(slot==9 && scene->simulation_time){   // C3: the vertex stage reads FC_GlobalTime too (FlowmapVertexOffset)
                        const auto produce=[&]{std::array<uint8_t,0xE0> bytes{};std::array<uint8_t,0xE0/4> written{};
                            ds3prod::ProduceFrameTime(scene->simulation_time->now,bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());
                            if(scene->per_frame_rows)ds3prod::ProducePerFrame(scene->simulation_time->now,scene->per_frame_rows->a.data(),scene->per_frame_rows->b.data(),
                                scene->per_frame_rows->blend,bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());
                            ProduceNativePerFrameGlobals(bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());
                            ProduceNativeSnowRows(*scene,bytes.data(),static_cast<uint32_t>(bytes.size()),written.data());   // I-0105
                            const auto ranges=NativeShaderWordRanges(bytes,written);
                            return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbPerFrame");};
                        shared_block=share({mesh.programs.get(),nullptr,prepared.recipe,(2u<<16)|(stage<<8)|slot},produce);
                    }
                    else if(slot==4){if(const auto& stored=NativeMeshInstanceBlock(model,mesh,prepared.fade,prepared.recipe,stage))shared_block=stored;}
                    else if(!stage && slot==0 && velocity_draw){   // 1186: Velo VS b0 {VC_MatrixWVP, VC_PrevMatrixWVP}
                        const auto produce=[&]{const ds3content::ShaderConstantRange range{0,scene->velocity->matrices};
                            return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,{&range,1},"cbMatrixWVP");};
                        shared_block=share({mesh.programs.get(),nullptr,prepared.recipe,(3u<<16)|(stage<<8)|slot},produce);}
                    else if(!stage && slot==0){if(const auto& stored=NativeMeshViewBlock(*scene,mesh,prepared.recipe))shared_block=stored;}
                    else if(!stage && slot==11){
                        if(model.texture_scroll){
                            const auto produce=[&]{const ds3content::ShaderConstantRange range{0,*model.texture_scroll};
                                return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,{&range,1},"cbTexScroll");};
                            // cP5: per programme recipe and model (the model's scroll rows are fixed for the scene), call-local.
                            shared_block=share({mesh.programs.get(),&model,prepared.recipe,(7u<<16)|(stage<<8)|slot},produce);
                        }
                    }
                    else if(slot==1){
                        if(!prepared.tiling.stages[stage?2:0]){complete=false;missing_constants|=1u<<(slot+stage*14);break;}
                        // PREPARE-REDUCTION cP5: a pure function of the row's tiling values and coverage and the programme recipe
                        // and stage; kept per Programs by the whole key (64 entries, oldest dropped).
                        const auto& tiling_bytes=prepared.tiling.values.bytes;const auto& tiling_written=prepared.tiling.values.written;
                        const auto same_tiling=[&](const NativeModelLayout::Programs::MatDynEntry& entry){
                            return entry.recipe==prepared.recipe && entry.stage==stage &&
                                std::equal(entry.bytes.begin(),entry.bytes.end(),tiling_bytes.begin(),tiling_bytes.end()) &&
                                std::equal(entry.written.begin(),entry.written.end(),tiling_written.begin(),tiling_written.end());};
                        std::shared_ptr<const ds3content::ShaderRegisterBlock> kept_tiling;
                        {const std::lock_guard hold(mesh.programs->matdyn_lock);
                            for(const auto& entry:mesh.programs->tiling_blocks)if(same_tiling(entry)){kept_tiling=entry.block;break;}}
                        shared_block=kept_tiling;
                        if(!shared_block){
                            const auto ranges=NativeShaderWordRanges(tiling_bytes,tiling_written);
                            block=PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,ranges,"cbTextureTilingScale");
                            {auto made=std::make_shared<const ds3content::ShaderRegisterBlock>(std::move(block));
                                const std::lock_guard hold(mesh.programs->matdyn_lock);auto& blocks=mesh.programs->tiling_blocks;
                                std::erase_if(blocks,same_tiling);if(blocks.size()>=64)blocks.erase(blocks.begin());
                                blocks.push_back({prepared.recipe,static_cast<uint32_t>(stage),false,0,
                                    std::vector<uint8_t>(tiling_bytes.begin(),tiling_bytes.end()),std::vector<uint8_t>(tiling_written.begin(),tiling_written.end()),made});
                                shared_block=std::move(made);}
                        }
                    }else if(stage && forward_draw && (slot==5 || slot==6)){
                        // 1114: forward PS cbLight/cbFog, the model's own light and fog entries.
                        const auto& entry=slot==5?model.forward_light->light:model.forward_light->fog;   // cP7
                        const ds3content::ShaderConstantRange range{0,entry};
                        block=PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,{&range,1},slot==5?"cbLight":"cbFog");
                    }else if(!stage && (slot==5 || slot==12 || (velocity_draw && slot==6))){
                        //817 owns the explicit current/auxiliary choice.815
                        //checks all four indices of the same physical CPU
                        //stream and exact selected VS before relative reads.
                        // 1186: b6 (Velo) = the previous pose version through the same route.
                        const auto* selected_palette=slot==5?pose_binding.object:slot==6?previous_binding.object:pose_binding.cancel;
                        // 1122: a current-route draw whose pose has no cancel (auxiliary absent: the engine never produces one
                        // for this model) still reads VS b12 (SAT_Equip/hair skinned VS: pos = b5 * b12 * v). The original leaves
                        // b12 from the preceding draw (64340 normal route binds b5 only). Explicit open policy: identity rows,
                        // the cancel the engine's own pose producer gives every pose without cloth composition.
                        ds3content::ModelPosePalette identity_cancel;
                        if(slot==12 && !selected_palette && pose_binding.kind==ds3content::ModelPoseBindingKind::Current && pose_binding.object){
                            identity_cancel.status=ds3content::ModelPoseStatus::Ready;identity_cancel.rows.assign(pose_binding.object->rows.size(),PoseRow{1,0,0,0,0,1,0,0,0,0,1,0});
                            selected_palette=&identity_cancel;}
                        // 1029: a dynamically indexed palette outside the two frozen
                        // character patterns (map pieces: one-row pose, no cancel
                        // palette) is bound padded with zeros to its declared
                        // cbuffer size, so every index the shader can address is
                        // backed (D3D11 out-of-range constant reads return zero).
                        size_t padded_bytes{};
                        if(dynamic_palettes && !palette_indices_valid && reads.ok && selected_palette){
                            const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),
                                [&](const auto& candidate){return candidate.name==resource.name;});
                            // 1059 characters: the skinned VS declares VC_aObjMatrix[2] but
                            // indexes the whole engine palette ((BLENDINDICES + matricesData.x)
                            // * 3, K-0401/K-0402) beyond its RDEF size, as D3D11 allowed.
                            // 1122: the same holds for b12 (indexed by the raw blend index * 3).
                            // 1184: never below the palette itself (skinned map objects reach 282 rows, MENUS-MAP 65).
                            // C4-1b review: the palette registers (VS b5, b6, b12) are bounded descriptor CBVs, which read zero
                            // past their view as D3D11 did, so the palette is bound at its own size (at least the declaration)
                            // instead of zero-padded to the 256 rows a byte index can address.
                            const size_t declared=declaration!=shader->constants.end() && declaration->declared_bytes<=65536?declaration->declared_bytes:0;
                            padded_bytes=std::max(declared,selected_palette->rows.size()*sizeof(PoseRow));
                        }
                        if(!reads.ok || !selected_palette || (dynamic_palettes && !palette_indices_valid && !padded_bytes)){
                            // 1028 witness: which palette input is absent for a selected row.
                            // 1070: also every 30th failure of a character part (asm_part), at most 12 more.
                            static LONG64 palette_samples{},asm_palette_failures{},asm_palette_samples{};
                            const bool asm_sample=model.asm_part>=0 && InterlockedIncrement64(&asm_palette_failures)%300==1 && InterlockedIncrement64(&asm_palette_samples)<=40;
                            if(asm_sample || InterlockedIncrement64(&palette_samples)<=12){char row[960]{};
                                const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),[&](const auto& candidate){return candidate.name==resource.name;});
                                sprintf_s(row,
                                "{\"event\":\"native_selected_palette_unavailable\",\"asm_part\":%d,\"declared\":%u,\"object_rows\":%zu,\"slot\":%u,\"reads\":%s,\"pose\":%s,\"palettes\":%s,\"pose_policy\":%s,\"object\":%s,\"cancel\":%s,\"dynamic\":%s,\"indices_valid\":%s,\"pose_rows\":%zu,\"entity\":%llu,\"model\":%llu,\"kind\":%d,\"cloth\":%d,\"auxiliary_present\":%d,\"compose\":%d,\"reference\":%d,\"cancel_status\":%d,\"cloth_enabled\":%d,\"policy_flags\":%u,\"family\":\"%.80s\"}\n",
                                model.asm_part,declaration!=shader->constants.end()?declaration->declared_bytes:0u,selected_palette?selected_palette->rows.size():size_t(0),
                                slot,reads.ok?"true":"false",draw_pose?"true":"false",draw_pose && draw_pose->palettes?"true":"false",
                                mesh.pose_policy?"true":"false",pose_binding.object?"true":"false",pose_binding.cancel?"true":"false",
                                dynamic_palettes?"true":"false",palette_indices_valid?"true":"false",draw_pose?draw_pose->matrices.size():0,
                                static_cast<unsigned long long>(model.entity),static_cast<unsigned long long>(model.model),
                                static_cast<int>(pose_binding.kind),draw_pose && draw_pose->palettes?int(draw_pose->palettes->cloth):-1,
                                draw_pose && draw_pose->palettes?int(draw_pose->palettes->auxiliary_present):-1,draw_pose && draw_pose->palettes?int(draw_pose->palettes->compose):-1,
                                draw_pose && draw_pose->palettes?int(draw_pose->palettes->reference):-1,draw_pose && draw_pose->palettes?int(draw_pose->palettes->cancel.status):-1,
                                mesh.pose_policy?int(mesh.pose_policy->cloth_enabled):-1,mesh.pose_policy?mesh.pose_policy->flags:0u,NativeFamilyKey(*mesh.programs).c_str());Log(row);}
                            ++unsupported_palettes;complete=false;break;
                        }
                        const auto produce=[&]{const auto& palette=selected_palette->rows;
                            std::vector<uint8_t> padded;
                            if(padded_bytes){padded.resize(padded_bytes,0);std::memcpy(padded.data(),palette.data(),palette.size()*sizeof(PoseRow));}
                            const auto bytes=padded_bytes?std::span<const uint8_t>(padded):
                                std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(palette.data()),palette.size()*sizeof(PoseRow));
                            const ds3content::ShaderConstantRange range{0,bytes};
                            auto packed=PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,{&range,1},
                                slot==5?"cbObjMatrix":slot==6?"cbPrevObjMatrix":"cbClothCancelObjMatrix");
                            if(packed.status==ds3content::ShaderConstantStatus::Ready){
                                // Keep the complete effective palette. The packer
                                // only checked reads; its RDEF-sized result cannot
                                // become the upload extent of a longer palette.
                                if(packed.bytes.size()<bytes.size())packed.bytes.resize(bytes.size());
                                std::copy(bytes.begin(),bytes.end(),packed.bytes.begin());
                            }
                            return packed;};
                        // cP5: per palette version (a model's pose palettes live as long as the scene), padding and programme
                        // recipe and stage; the per-row identity cancel is a local object and is never shared.
                        if(selected_palette==&identity_cancel)block=produce();
                        else shared_block=share({mesh.programs.get(),selected_palette,prepared.recipe,(uint64_t(padded_bytes)<<32)|(4u<<16)|(stage<<8)|slot},produce);
                    }else shared_block=share({mesh.programs.get(),nullptr,prepared.recipe,(5u<<16)|(stage<<8)|slot},[&]{
                        return PrepareNativeProgramRegister(*mesh.programs,prepared.recipe,stage,slot,{});});
                    const auto& resolved=shared_block?*shared_block:block;
                    if(slot>=14 || resolved.status!=ds3content::ShaderConstantStatus::Ready || !resolved.required || resolved.bytes.empty()){
                        // 1030 witness: the named constant block a selected row lacks.
                        static LONG64 constant_samples{},asm_numeric_samples{};
                        // 1110: also the first 12 character-part cbMtdParam (b3) failures, with their unproduced words.
                        const bool asm_numeric=slot==3 && model.asm_part>=0 && InterlockedIncrement64(&asm_numeric_samples)<=12;
                        if(asm_numeric || InterlockedIncrement64(&constant_samples)<=12){
                            const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),
                                [&](const auto& candidate){return candidate.name==resource.name;});
                            std::string family;for(const auto ch:mesh.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                            // 1042: for cbInstanceData, the read words the instance record leaves unproduced.
                            std::string unproduced;
                            if(slot==4 && reads.ok && declaration!=shader->constants.end() && declaration->declared_bytes<=65536){
                                const auto& instance=model.instances[prepared.fade];
                                const auto mask=ds3reads::FloatMask(reads,slot,declaration->declared_bytes,static_cast<bool*>(nullptr));
                                for(size_t word=0;word<mask.size() && unproduced.size()<900;++word){
                                    const size_t record_word=word%(ds3prod::instance_record_bytes/4);
                                    if(!mask[word] || instance.written[record_word])continue;
                                    const auto covering=std::find_if(declaration->variables.begin(),declaration->variables.end(),[&](const auto& variable){
                                        return word*4>=variable.byte_offset && word*4<uint64_t(variable.byte_offset)+variable.bytes;});
                                    char item[96]{};sprintf_s(item,"%s\"%zu:%.40s\"",unproduced.empty()?"":",",word*4,
                                        covering!=declaration->variables.end()?covering->name.c_str():"?");unproduced+=item;}
                            }
                            if(slot==3 && reads.ok && declaration!=shader->constants.end() && declaration->declared_bytes<=65536){
                                const auto mask=ds3reads::FloatMask(reads,slot,declaration->declared_bytes,static_cast<bool*>(nullptr));
                                for(size_t word=0;word<mask.size() && unproduced.size()<900;++word){
                                    if(!mask[word] || std::any_of(mesh.numeric_writes.begin(),mesh.numeric_writes.end(),[&](const auto& write){
                                        return word*4>=write.byte_offset && word*4+4<=uint64_t(write.byte_offset)+write.bytes;}))continue;
                                    const auto covering=std::find_if(declaration->variables.begin(),declaration->variables.end(),[&](const auto& variable){
                                        return word*4>=variable.byte_offset && word*4<uint64_t(variable.byte_offset)+variable.bytes;});
                                    char item[96]{};sprintf_s(item,"%s\"%zu:%.40s\"",unproduced.empty()?"":",",word*4,
                                        covering!=declaration->variables.end()?covering->name.c_str():"?");unproduced+=item;}
                            }
                            char row[1536]{};sprintf_s(row,"{\"event\":\"native_selected_constant_unavailable\",\"stage\":%zu,\"slot\":%u,\"name\":\"%.80s\",\"status\":%u,\"required\":%s,\"declared_bytes\":%u,\"family\":\"%.120s\",\"unproduced\":[%.1000s]}\n",
                                stage,slot,resource.name.c_str(),static_cast<unsigned>(resolved.status),resolved.required?"true":"false",
                                declaration!=shader->constants.end()?static_cast<unsigned>(declaration->declared_bytes):0u,family.c_str(),unproduced.c_str());Log(row);}
                        if(slot<14)missing_constants|=1u<<(slot+stage*14);complete=false;break;
                    }
                    // 1095 witness: the pixel constant values a body mesh (asm part 0/1) is drawn with.
                    if(stage && (model.asm_part==0 || model.asm_part==1)){static LONG64 body_constant_samples{};
                        if(InterlockedIncrement64(&body_constant_samples)<=24){
                            const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),[&](const auto& candidate){return candidate.name==resource.name;});
                            std::string row="{\"event\":\"native_body_pixel_constants\",\"part\":"+std::to_string(model.asm_part)+",\"mesh\":"+std::to_string(mesh.current.ordinal)+
                                ",\"slot\":"+std::to_string(slot)+",\"buffer\":\""+resource.name.substr(0,40)+"\",\"values\":{";
                            if(declaration!=shader->constants.end())for(const auto& variable:declaration->variables){
                                if(row.size()>3000)break;
                                char cell[160]{};float v[4]{};const auto count=std::min<uint32_t>(4,variable.bytes/4);
                                for(uint32_t k=0;k<count;++k)if(variable.byte_offset+4*k+4<=resolved.bytes.size())std::memcpy(&v[k],resolved.bytes.data()+variable.byte_offset+4*k,4);
                                sprintf_s(cell,"%s\"%.40s@%u\":[%g,%g,%g,%g]",row.back()=='{'?"":",",variable.name.c_str(),variable.byte_offset,v[0],v[1],v[2],v[3]);row+=cell;}
                            row+="}}\n";Log(row.c_str());}}
                    // 1180 witness: the whole texture tiling array of a forward hair draw (element N of Texture2D_N).
                    if(stage && forward_draw && offscreen && slot==1 && mesh.programs->program_path.find(L"Hair")!=std::wstring::npos){static LONG64 tiling_samples{};
                        if(InterlockedIncrement64(&tiling_samples)<=20){std::string values;for(size_t w=0;w+4<=resolved.bytes.size() && w<64;w+=4){float v{};std::memcpy(&v,resolved.bytes.data()+w,4);char cell[24]{};sprintf_s(cell,"%s%g",values.empty()?"":",",v);values+=cell;}
                            const std::string row="{\"event\":\"native_hair_tiling\",\"mesh\":"+std::to_string(mesh.current.ordinal)+",\"model\":"+std::to_string(prepared.model)+",\"values\":["+values+"]}\n";Log(row.c_str());}}
                    // 1177 witness: the pixel constants of a forward hair draw in a menu view (hair drawn but invisible).
                    if(stage && forward_draw && offscreen && mesh.programs->program_path.find(L"Hair")!=std::wstring::npos){static LONG64 hair_constant_samples{};
                        if(InterlockedIncrement64(&hair_constant_samples)<=12){
                            const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),[&](const auto& candidate){return candidate.name==resource.name;});
                            std::string row="{\"event\":\"native_hair_pixel_constants\",\"mesh\":"+std::to_string(mesh.current.ordinal)+",\"slot\":"+std::to_string(slot)+",\"buffer\":\""+resource.name.substr(0,40)+"\",\"values\":{";
                            if(declaration!=shader->constants.end())for(const auto& variable:declaration->variables){
                                if(row.size()>3500)break;
                                char cell[160]{};float v[4]{};const auto count=std::min<uint32_t>(4,variable.bytes/4);
                                for(uint32_t k=0;k<count;++k)if(variable.byte_offset+4*k+4<=resolved.bytes.size())std::memcpy(&v[k],resolved.bytes.data()+variable.byte_offset+4*k,4);
                                sprintf_s(cell,"%s\"%.40s@%u\":[%g,%g,%g,%g]",row.back()=='{'?"":",",variable.name.c_str(),variable.byte_offset,v[0],v[1],v[2],v[3]);row+=cell;}
                            row+="}}\n";Log(row.c_str());}}
                    // C4-1b review: a root CBV has no bound (a relative read past its payload reads other data, not zero), so a row
                    // whose programme indexes a root-CBV register dynamically is refused, witnessed once per programme, recipe,
                    // stage and register; the bounded registers (NativeModelBoundedConstant) read zero past their view. A programme
                    // without a read analysis (reads.ok false) is witnessed too: its indexing is unknown.
                    if(!reads.ok || (reads.dynamic.contains(slot) && !NativeModelBoundedConstant(static_cast<uint32_t>(stage),slot))){
                        static std::mutex unbounded_lock;static std::set<std::tuple<const void*,uint32_t,size_t,uint32_t>> unbounded_seen;bool fresh{};
                        {const std::lock_guard hold(unbounded_lock);fresh=unbounded_seen.size()<256 && unbounded_seen.emplace(mesh.programs.get(),prepared.recipe,stage,slot).second;}
                        if(fresh){char row[320]{};sprintf_s(row,"{\"event\":\"native_constant_unbounded_dynamic_read\",\"stage\":%zu,\"slot\":%u,\"name\":\"%.40s\",\"reads\":%s,\"family\":\"%.120s\"}\n",
                            stage,slot,resource.name.c_str(),reads.ok?"true":"false",NativeFamilyKey(*mesh.programs).c_str());Log(row);}
                        if(reads.ok){++unbounded_dynamic;complete=false;break;}}
                    // C4-1: a frame register's block is bound as the pass's root CBV; its packed bytes stay with the draw for the
                    // declaration check only, and the longest of each kind becomes the view's canonical block.
                    if(const auto root=NativeModelFrameRootOf(static_cast<uint32_t>(stage),slot);root<FrameRootCount){
                        if(!shared_block)shared_block=std::make_shared<const ds3content::ShaderRegisterBlock>(std::move(block));
                        input.constants.push_back({shader_stage,slot,{},note_frame_block(shared_block,static_cast<uint32_t>(stage),slot,velocity_draw,forward_draw),true});
                    }else{
                    kprofile::AddIfActive(kprofile::PrepConstantBlocks);kprofile::AddIfActive(kprofile::PrepConstantBytes,resolved.bytes.size());
                    if(shared_block)input.constants.push_back({shader_stage,slot,{},std::shared_ptr<const std::vector<uint8_t>>(shared_block,&shared_block->bytes)});
                    else input.constants.push_back({shader_stage,slot,std::move(block.bytes)});}
                }else if(resource.input_type==3){kprofile::SwitchIfActive(kprofile::PrepSamplersSrvs);
                    // Common VS/DS/PS s0..12 table of 10AAD0/10AB80 (sampler789):
                    // banks 12,12,12,12,14,12,0,3,1,4,12,13,15; s13..15 unwritten.
                    // Map materials read s5 (SS_GIMap, Native1032). Witnesses781
                    // covered s0..2/s8 of the same table.
                    const auto slot=resource.shader_register;
                    constexpr uint32_t common_banks[13]{12,12,12,12,14,12,0,3,1,4,12,13,15};
                    if(resource.count!=1 || slot>12){++unsupported_samplers;complete=false;break;}
                    const auto bank=common_banks[slot];
                    input.samplers.push_back({shader_stage,slot,scene->sampler_bank->descriptors[bank]});
                }else{
                    kprofile::SwitchIfActive(kprofile::PrepSamplersSrvs);
                    const auto& textures_for_shader=mesh.shader_textures[prepared.recipe][stage];
                    const auto binding=std::find_if(textures_for_shader.registers.begin(),textures_for_shader.registers.end(),
                        [&](const auto& candidate){return candidate.slot==resource.shader_register;});
                    if(textures_for_shader.status!=ds3content::ModelTextureStatus::Ready || binding==textures_for_shader.registers.end()){
                        TextureWitness(mesh,resource.name,resource.shader_register,stage,textures_for_shader.status!=ds3content::ModelTextureStatus::Ready?1u:2u,static_cast<unsigned>(textures_for_shader.status),offscreen);
                        if(model.effect)EffectModelTextureWitness(model.model,resource.name,static_cast<unsigned>(textures_for_shader.status),textures_for_shader.status!=ds3content::ModelTextureStatus::Ready?1u:2u);
                        ++missing_textures;complete=false;break;
                    }
                    std::shared_ptr<const bootstrap::NativeTextureRecord> texture;
                    if(binding->source==ds3content::ModelTextureSource::Scene){
                        if(binding->binding>=mesh.scene_texture_bindings.size() ||
                           mesh.scene_texture_bindings[binding->binding].slot!=resource.shader_register){++missing_textures;complete=false;break;}
                        texture=mesh.scene_texture_bindings[binding->binding].texture;
                        // I-0107 (final review I1, step review M1): the light grid is the main camera's; an offscreen view
                        // reads the empty light buffers, as its words are zero.
                        if(offscreen && resource.shader_register>=59 && resource.shader_register<=62 && scene->common_images)
                            texture=scene->common_images->empty_light_buffers[resource.shader_register-59];
                        // I-0113 tranche 14: the region cubes 75B30 binds from the keys 5C0E0 gave the object (the main view's); tranche
                        // 35: a menu view's {1, 10} cube (NativeMenuRegionIbl, set at its admission).
                        if((resource.shader_register==30 || resource.shader_register==31) && model.region_ibl && model.region_ibl->bound[resource.shader_register-30])
                            texture=model.region_ibl->bound[resource.shader_register-30];
                        // I-0113 tranche 22 (gap 21): 13B450 binds the pair's min(Q+1A6, 1); D06E10 draws a LOD crossfade's second LOD
                        // with its branch argument 1 (the intent's branch), INFERRED to be the descriptor byte +0x0B that 5C0E0 copies to
                        // Q+1A6: the outgoing LOD takes the inverse pattern, so the two LODs' dithers cover each other. By register,
                        // as 13B450 binds it and the base texture is admitted (08_model_programmes.inc).
                        if(resource.shader_register==38 && scene->common_images && model.geometry_fades.intents[prepared.fade].branch==1 &&
                           scene->common_images->fade_dither[1])texture=scene->common_images->fade_dither[1];
                    }else texture=mesh.texture_bindings.at(binding->binding).texture;
                    if(!texture || !texture->resident || !texture->resource || !texture->view){TextureWitness(mesh,resource.name,resource.shader_register,stage,texture?4u:3u,0,offscreen);
                        if(model.effect)EffectModelTextureWitness(model.model,resource.name,0,texture?4u:3u);++missing_textures;complete=false;break;}
                    input.srvs.push_back({shader_stage,resource.shader_register,texture->view.handle,texture->resource,texture});kprofile::AddIfActive(kprofile::PrepSrvs);
                    if(srv_witness && srv_names.size()<900){std::string path;   // 1179: and the authored FLVER path
                        if(binding->source!=ds3content::ModelTextureSource::Scene)for(const auto ch:mesh.texture_bindings.at(binding->binding).input.path){if(path.size()>70)break;path.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'/');}
                        char cell[320]{};sprintf_s(cell,"%s{\"s\":%zu,\"r\":%u,\"n\":\"%.40s\",\"t\":\"%.80s\",\"w\":%u,\"f\":%u,\"p\":\"%s\"}",srv_names.empty()?"":",",stage,resource.shader_register,resource.name.c_str(),texture->content?texture->content->name.c_str():"(no content)",texture->content?texture->content->width:0u,texture->content?static_cast<unsigned>(texture->content->format):0u,path.c_str());srv_names+=cell;}
                }
            }
        }
        kprofile::SwitchIfActive(kprofile::PrepRowSetup);
        // W7 TS-4b: a tessellated row (the ModelTess family, a 3-control-point patch list) binds its hull and domain registers
        // as the tess root's CBVs: b5 cbTessScene (the view's), b6 cbPNTriangles (the material's), domain b8 cbSceneParam (the
        // view's image). Any other hull or domain resource, or a block not made, leaves the row incomplete (witnessed).
        if(complete && tess_row){make_tess_view();
            input.shape.family=ModelRootFamily::ModelTess;input.shape.topology=ModelTopologyClass::Patch3;
            for(size_t stage=0;stage<2 && complete;++stage){
                const auto& shader=mesh.programs->tess_interfaces[prepared.recipe][stage];
                if(!shader){++missing_interfaces;complete=false;break;}
                for(const auto& resource:shader->resources){const auto slot=resource.shader_register;
                    std::shared_ptr<const std::vector<uint8_t>> payload;
                    if(resource.input_type==0 && resource.count==1)payload=slot==5?tess_scene_block:slot==6?mesh.programs->pn_triangles:stage && slot==8?tess_camera_block:nullptr;
                    if(!payload){complete=false;static LONG64 unbound{};if(InterlockedIncrement64(&unbound)<=16){char row[320]{};
                        sprintf_s(row,"{\"event\":\"native_tess_register_unbound\",\"stage\":\"%s\",\"slot\":%u,\"input_type\":%u,\"name\":\"%.60s\",\"family\":\"%.100s\"}\n",
                            stage?"domain":"hull",slot,resource.input_type,resource.name.c_str(),NativeFamilyKey(*mesh.programs).c_str());Log(row);}break;}
                    input.constants.push_back({stage?ds3content::ShaderStage::Domain:ds3content::ShaderStage::Hull,slot,{},std::move(payload)});}}
            if(complete){static LONG64 tess_draws{};if(InterlockedIncrement64(&tess_draws)<=16){char row[320]{};
                sprintf_s(row,"{\"event\":\"native_tess_draw\",\"recipe\":%u,\"engine_pass\":%u,\"indices\":%u,\"offscreen\":%s,\"family\":\"%.100s\"}\n",
                    prepared.recipe,prepared.engine_pass,mesh.geometry.index_count,offscreen?"true":"false",NativeFamilyKey(*mesh.programs).c_str());Log(row);}}}
        if(!complete && model.asm_part>=0){static LONG64 asm_incomplete{};const auto n=InterlockedIncrement64(&asm_incomplete);
            if(n<=40 || n%600==0){char row[400]{};sprintf_s(row,"{\"event\":\"native_asm_part_draw_incomplete\",\"count\":%lld,\"part\":%d,\"mesh\":%u,\"stage\":%u,\"resource\":\"%.80s\",\"register\":%u,\"input_type\":%u,\"entity\":%llu}\n",
                n,model.asm_part,mesh.current.ordinal,failed_stage,failed_resource?failed_resource:"(interface)",failed_register,failed_input,static_cast<unsigned long long>(model.entity));Log(row);}}
        // 1177 witness: the textures a menu view's forward draw (hair, brows) samples.
        if(complete && offscreen && forward_draw){if(InterlockedIncrement64(&hair_srv_samples)<=12){std::string family;
            for(const auto ch:mesh.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
            std::string elements;for(const auto& e:mesh.layout->vertex_elements){char cell[96]{};sprintf_s(cell,"%s\"%.24s%u:%u:%u:%u\"",elements.empty()?"":",",e.semantic.c_str(),e.semantic_index,static_cast<unsigned>(e.format),e.input_slot,e.byte_offset);elements+=cell;}
            std::string sampler;if(scene->sampler_bank){const auto& s=scene->sampler_bank->descriptors[12];char cell[200]{};   // 1180: s0 (bank 12) of the hair PS
                sprintf_s(cell,"\"filter\":%u,\"address\":[%u,%u,%u],\"bias\":%g,\"min_lod\":%g,\"max_lod\":%g,\"border\":[%g,%g,%g,%g]",static_cast<unsigned>(s.Filter),static_cast<unsigned>(s.AddressU),static_cast<unsigned>(s.AddressV),static_cast<unsigned>(s.AddressW),s.MipLODBias,s.MinLOD,s.MaxLOD,s.BorderColor[0],s.BorderColor[1],s.BorderColor[2],s.BorderColor[3]);sampler=cell;}
            // I-0113 tranche 30 (the Quit Game portrait's dark hair): the PS blocks b3 (cbMtdParam: the hair's specular), b4 (the instance:
            // mulCol, the SH ambient aSh, the cube maps' colour scales) and b5 (cbLight) as hex, against the original's named_oracle ps_cb3..5.
            std::string blocks;for(const uint32_t reg:{3u,4u,5u}){std::string hex="null";
                for(const auto& b:input.constants)if(b.stage==ds3content::ShaderStage::Pixel && b.shader_register==reg){const auto& payload=b.Payload();hex="\""+HexBytes(payload.data(),payload.size())+"\"";break;}
                blocks+=",\"ps_b"+std::to_string(reg)+"\":"+hex;}
            const std::string row="{\"event\":\"native_offscreen_forward_sources\",\"mesh\":"+std::to_string(mesh.current.ordinal)+",\"family\":\""+family.substr(0,80)+"\",\"s0\":{"+sampler+"},\"elements\":["+elements+"],\"srvs\":["+srv_names+"]"+blocks+"}\n";Log(row.c_str());}}
        if(complete && model.asm_part>=0 && model.asm_part<2){
            if(const auto n=InterlockedIncrement64(&asm_srv_samples);n<=24 || n%600==0){std::string family;
                for(const auto ch:mesh.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                std::string row="{\"event\":\"native_asm_part_draw_sources\",\"part\":"+std::to_string(model.asm_part)+",\"pose_generation\":"+std::to_string(model.pose?model.pose->generation:0)+",\"mesh\":"+std::to_string(mesh.current.ordinal)+
                    ",\"family\":\""+family.substr(0,120)+"\",\"srvs\":["+srv_names+"]}\n";Log(row.c_str());}}
        // I-0113 tranche 18 (gap 21, the hair in a warp): a word of a PS block as the draw binds it (NaN when absent), and the row
        // also each time b4's ditherFadeAlpha (+0x100) moves by a twentieth, at most 640 rows, so a fade's steps are seen.
        const auto ps_word=[&](uint32_t reg,size_t offset){for(const auto& b:input.constants)if(b.stage==ds3content::ShaderStage::Pixel && b.shader_register==reg){
                const auto& payload=b.Payload();if(offset+4>payload.size())break;float v{};std::memcpy(&v,payload.data()+offset,4);return v;}
            return std::numeric_limits<float>::quiet_NaN();};
        static std::atomic<bool> named_full{};   // step review L6: no lock once the 640 rows are written (tranche 21: 256 before the mesh joined the key)
        if(named_witness && !named_full.load(std::memory_order_relaxed)){bool fresh{};const float dither_fade=ps_word(4,0x100);
            const int fade_step=std::isfinite(dither_fade)?static_cast<int>(std::lround(dither_fade*20.f)):-1;
            {const std::lock_guard hold(chair_hunt::seen_lock);
                fresh=chair_hunt::named_seen.size()<640 && chair_hunt::named_seen.emplace(mesh.current.material,mesh.current.ordinal,prepared.engine_pass,prepared.recipe,fade_step).second;
                if(chair_hunt::named_seen.size()>=640)named_full=true;}
            if(fresh){std::string family;for(const auto ch:mesh.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                // Review (the Handmaid chair): PS b2, b3 and b4 (the first 0x80 bytes each), and the material policy the pass rows took.
                std::string words;for(const uint32_t reg:{2u,3u,4u}){if(!words.empty())words+="|";bool any{};
                    for(const auto& b:input.constants)if(b.stage==ds3content::ShaderStage::Pixel && b.shader_register==reg){any=true;
                        const auto payload=b.Payload();for(size_t o=0;o+4<=payload.size() && o<0x80;o+=4){float v{};std::memcpy(&v,payload.data()+o,4);char cell[32]{};
                            sprintf_s(cell,"%s%g",o==0?"":",",v);words+=cell;}break;}
                    if(!any)words+="-";}
                std::string policy="null";if(mesh.programs->policy){const auto& p=*mesh.programs->policy;char cell[160]{};
                    sprintf_s(cell,"{\"alpha_ref\":%u,\"blend\":%u,\"flags\":%u,\"transparent\":%s,\"forward\":%s,\"class\":%u}",p.alpha_ref,p.blend_mode,p.flags,p.transparent?"true":"false",p.forward?"true":"false",mesh.material_class?*mesh.material_class:0u);policy=cell;}
                const std::string row="{\"event\":\"native_named_draw_sources\",\"model\":\""+NativeModelDataName(model.model)+"\",\"mesh\":"+std::to_string(mesh.current.ordinal)+
                    ",\"engine_pass\":"+std::to_string(prepared.engine_pass)+",\"recipe\":"+std::to_string(static_cast<uint32_t>(prepared.recipe))+",\"complete\":"+(complete?"true":"false")+
                    ",\"family\":\""+family.substr(0,140)+"\",\"policy\":"+policy+",\"ps_b2_b3_b4\":\""+words+"\",\"srvs\":["+srv_names+"]"+
                    // I-0113 tranche 21 (gap 21: which of the hair's meshes each fading row draws, natively against the original;
                    // meshes sharing a material were one row before): the draw's index range, face set cull class and mesh flags (63CB0's mesh filter), against named_oracle's count, start and raster and reference_mesh_loop_filter.
                    ",\"index_count\":"+std::to_string(mesh.slice.index_count)+",\"index_offset\":"+std::to_string(mesh.slice.index_offset)+",\"cull_class\":"+std::to_string(mesh.slice.cull_class)+",\"mesh_flags\":"+std::to_string(mesh.current.flags);
                // I-0113 tranche 18: the words the _[A] forward programmes' dither discard reads (b2 +0x0 FC_isDitherAlpha, b7 +0x20
                // FC_DitherEnable, b4 +0x100 ditherFadeAlpha), b4's mulCol.w (+0x7C), b3's FC_AlphaRef, the colour blend and depth write.
                const auto& rt=input.blend.RenderTarget[0];uint32_t dither_alpha_bits{};
                for(const auto& b:input.constants)if(b.stage==ds3content::ShaderStage::Pixel && b.shader_register==2 && b.Payload().size()>=4){std::memcpy(&dither_alpha_bits,b.Payload().data(),4);break;}
                // The final review's L2: an absent word (NaN) is written null, so every row stays JSON.
                const auto number=[](float v){std::string s="null";if(std::isfinite(v)){char c[32]{};sprintf_s(c,"%g",v);s=c;}return s;};
                char tail[400]{};sprintf_s(tail,",\"dither\":{\"is_dither_alpha\":%u,\"enable\":%s,\"fade_alpha\":%s},\"mulcol_w\":%s,\"alpha_ref\":[%s,%s],\"blend\":[%d,%d,%d],\"depth_write\":%d}\n",
                    dither_alpha_bits,number(ps_word(7,0x20)).c_str(),number(dither_fade).c_str(),number(ps_word(4,0x7C)).c_str(),number(ps_word(3,0x50)).c_str(),number(ps_word(3,0x54)).c_str(),
                    rt.BlendEnable,rt.SrcBlend,rt.DestBlend,int(input.depth_stencil.DepthWriteMask));
                Log((row+tail).c_str());}}
        // I-0113 tranche 7 (ZERO-CONSTANTS-CENSUS §5, zero-census.flag): once per (model, mesh, recipe) of the main view, at most 8000
        // rows: the PS's DXBC checksum (as the original's reference_zero_census) and, for PS b2, b3, b4 and b7, the bound bytes and the
        // words the programme reads (a hex digit per four words, bit i the word 4n+i), for Compare-I113ZeroCensus.py.
        {static const bool zero_census=ModuleFlag(L"zero-census.flag");
            if(zero_census && complete && !offscreen && input.program_set && input.program_set->pixel.bytecode.size()>=12){
                const auto census_name=NativeModelDataName(model.model);static std::mutex census_lock;
                static std::set<std::tuple<std::string,uint32_t,uint32_t>> census_seen;bool census_fresh{};
                {const std::lock_guard census_hold(census_lock);census_fresh=census_seen.size()<8000 && census_seen.emplace(census_name,mesh.current.ordinal,static_cast<uint32_t>(prepared.recipe)).second;}
                const auto& census_shader=mesh.programs->interfaces[prepared.recipe][1];
                if(census_fresh && census_shader){const auto& census_reads=mesh.programs->ReadsOf(prepared.recipe,1);uint64_t census_checksum{};
                    std::memcpy(&census_checksum,input.program_set->pixel.bytecode.data()+4,8);
                    std::string census_clean,census_family;for(const auto census_ch:census_name)census_clean.push_back(census_ch>=32 && census_ch<127 && census_ch!='"' && census_ch!='\\'?census_ch:'?');
                    for(const auto census_ch:mesh.programs->program_path)census_family.push_back(census_ch>=32 && census_ch<127 && census_ch!='"' && census_ch!='\\'?static_cast<char>(census_ch):'?');
                    char census_head[300]{};sprintf_s(census_head,"\",\"mesh\":%u,\"engine_pass\":%u,\"recipe\":%u,\"count\":%u,\"ps_dxbc\":\"%016llX\",\"family\":\"",
                        mesh.current.ordinal,prepared.engine_pass,static_cast<uint32_t>(prepared.recipe),mesh.geometry.index_count,
                        static_cast<unsigned long long>(_byteswap_uint64(census_checksum)));
                    std::string census_row="{\"event\":\"native_zero_census\",\"model\":\""+census_clean.substr(0,80)+census_head+census_family.substr(0,140)+"\"";
                    for(const uint32_t census_slot:{2u,3u,4u,7u}){
                        const auto census_res=std::find_if(census_shader->resources.begin(),census_shader->resources.end(),[&](const auto& census_resource){return census_resource.input_type==0 && census_resource.shader_register==census_slot;});
                        const auto census_bind=std::find_if(input.constants.begin(),input.constants.end(),[&](const auto& census_binding){return census_binding.stage==ds3content::ShaderStage::Pixel && census_binding.shader_register==census_slot;});
                        if(census_res==census_shader->resources.end() || census_bind==input.constants.end())continue;
                        const auto census_decl=std::find_if(census_shader->constants.begin(),census_shader->constants.end(),[&](const auto& census_constant){return census_constant.name==census_res->name;});
                        std::string census_used;
                        if(census_reads.ok && census_decl!=census_shader->constants.end() && census_decl->declared_bytes<=65536){
                            const auto census_mask=ds3reads::FloatMask(census_reads,census_slot,census_decl->declared_bytes,static_cast<bool*>(nullptr));
                            for(size_t census_word=0;census_word<census_mask.size();census_word+=4){unsigned census_nibble{};for(size_t census_k=0;census_k<4 && census_word+census_k<census_mask.size();++census_k)if(census_mask[census_word+census_k])census_nibble|=1u<<census_k;
                                census_used.push_back("0123456789abcdef"[census_nibble]);}}
                        const auto& census_payload=census_bind->Payload();
                        census_row+=",\"b"+std::to_string(census_slot)+"\":{\"bytes\":\""+HexBytes(census_payload.data(),(std::min)(census_payload.size(),size_t(4096)))+"\",\"used\":\""+census_used+"\"}";}
                    Log((census_row+"}\n").c_str());}}}
        LARGE_INTEGER draw_start{},draw_end{};QueryPerformanceCounter(&draw_start);native_prep_ticks[1]+=draw_start.QuadPart-resources_start.QuadPart;
        // m50 sky (S4): the first 24 forward draws of map pieces, by model and mesh: complete or not, and the words that can make
        // their pixels vanish (PS b3 FC_AlphaRef +0x50, PS b7 FC_DitherEnable +0x20, PS b2 isDitherAlpha +0x0 and FC_DynamicDiffuseColor
        // +0x180, b4 instance mulCol +0x70 and ditherFadeAlpha +0x100).
        static const bool forward_diagnostics=ModuleFlag(L"light-observe.flag");   // review: diagnostics only with light-observe.flag
        if(forward_diagnostics && forward_draw && !offscreen){static std::mutex forward_map_lock;static std::set<std::pair<uintptr_t,uint32_t>> forward_map_seen;bool fresh{};const auto name=NativeModelDataName(model.model);
            if(!name.empty() && name[0]=='m'){const std::lock_guard hold(forward_map_lock);fresh=forward_map_seen.size()<24 && forward_map_seen.emplace(model.model,mesh.current.ordinal).second;}
            if(fresh){const auto word=[&](ds3content::ShaderStage s,uint32_t slot,uint32_t offset){float v=-999.f;
                    for(const auto& binding:input.constants)if(binding.stage==s && binding.shader_register==slot && binding.Payload().size()>=offset+4){std::memcpy(&v,binding.Payload().data()+offset,4);break;}
                    return v;};
                BYTE flag20{},provider54{},provider41{};uintptr_t provider{};Read(mesh.current.material+0xD0+0x20,&flag20,1);Read(mesh.current.material+0xD0+0x360,&provider,8);
                if(provider){Read(provider+0x54,&provider54,1);Read(provider+0x41,&provider41,1);}
                uintptr_t images[2]{};Read(mesh.current.material+0x90,images,16);   // 140850: param_1[0x12] (count) and [0x13] (the per-slot CPU images)
                const auto P=ds3content::ShaderStage::Pixel;std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                char row[600]{};sprintf_s(row,"{\"event\":\"native_forward_map_draw\",\"model\":\"%.60s\",\"mesh\":%u,\"engine_pass\":%u,\"recipe\":%u,\"complete\":%s,\"failed\":[%u,\"%.40s\",%u],"
                    "\"alpha_ref\":[%g,%g],\"dither_enable\":%g,\"dither_alpha\":%g,\"diffuse\":[%g,%g,%g],\"mulcol\":[%g,%g,%g,%g],\"fade\":%g,\"textures\":%zu,\"flag20\":%u,\"provider\":%s,\"provider_flags\":[%u,%u],\"dyn_images\":[%llu,%llu]}\n",
                    clean.c_str(),mesh.current.ordinal,prepared.engine_pass,prepared.recipe,complete?"true":"false",failed_stage,failed_resource?failed_resource:"",failed_register,
                    word(P,3,0x50),word(P,3,0x54),word(P,7,0x20),word(P,2,0),word(P,2,0x180),word(P,2,0x184),word(P,2,0x188),
                    word(P,4,0x70),word(P,4,0x74),word(P,4,0x78),word(P,4,0x7C),word(P,4,0x100),input.srvs.size(),flag20,provider?"true":"false",provider54,provider41,static_cast<unsigned long long>(images[0]),static_cast<unsigned long long>(images[1]));Log(row);}}
        // White equipment parts (TestAnim shins, the summon-sign cape and shield): the first 64 equipment draws (ChrAsm parts) by
        // material, with the MTD, the row, the bound textures and the generated words of b2 (light-observe.flag).
        if(forward_diagnostics && model.asm_part>=0 && !offscreen && mesh.material){static std::mutex part_lock;static std::set<std::tuple<uintptr_t,uint32_t,uint32_t>> part_seen;static std::atomic<bool> part_full{};bool fresh{};
            if(!part_full.load(std::memory_order_relaxed)){const std::lock_guard hold(part_lock);fresh=part_seen.size()<64 && part_seen.emplace(mesh.current.material,prepared.engine_pass,prepared.recipe).second;if(part_seen.size()>=64)part_full.store(true);}
            if(fresh){const auto ascii=[](const std::wstring& text){std::string out;for(const auto ch:text)if(out.size()<70)out.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');return out;};
                std::string part_textures;for(const auto& t:mesh.material->texture_inputs){if(part_textures.size()>400)break;part_textures+=(part_textures.empty()?"":",")+ascii(t.type)+"="+ascii(t.path.substr(t.path.find_last_of(L"\\/")==std::wstring::npos?0:t.path.find_last_of(L"\\/")+1));}
                std::string generated;if(mesh.programs && mesh.programs->generated_parameters){const auto& g=*mesh.programs->generated_parameters;
                    for(size_t i=0;i<g.words.size() && i<12;++i){char cell[24]{};sprintf_s(cell,"%s%g",i?",":"",std::bit_cast<float>(g.words[i]));generated+=cell;}}
                const uint64_t capability[2]{mesh.material_masks?mesh.material_masks->first:0,mesh.material_masks?mesh.material_masks->second:0};
                const uint64_t group[2]{model.shader_capabilities?(*model.shader_capabilities)[0]:0,model.shader_capabilities?(*model.shader_capabilities)[1]:0};
                char head[600]{};sprintf_s(head,"{\"event\":\"native_part_draw\",\"asm_part\":%d,\"model\":\"%.40s\",\"mesh\":%u,\"engine_pass\":%u,\"recipe\":%u,\"complete\":%s,\"srvs\":%zu,\"material\":\"%s\",\"mtd\":\"%s\",\"capability\":[\"%llX\",\"%llX\"],\"group\":[\"%llX\",\"%llX\"],\"authored_generated\":[%s],",
                    model.asm_part,NativeModelDataName(model.model).c_str(),mesh.current.ordinal,prepared.engine_pass,prepared.recipe,complete?"true":"false",input.srvs.size(),ascii(mesh.material->name).c_str(),ascii(mesh.material->mtd).c_str(),
                    capability[0],capability[1],group[0],group[1],generated.c_str());
                Log((std::string(head)+"\"textures\":\""+part_textures+"\"}\n").c_str());}}
        if(complete){std::shared_ptr<const OwnedNativeModelDraw> draw;
            // Fable review: one row whose draw cannot be made (a pipeline the device refuses, a binding out of range) is counted
            // and skipped, as the decal rows are; it no longer faults the render worker and ends the process.
            // I-0090: a rebuilt kept row offers its kept draw's state; read-only while the chunks run.
            try{draw=PrepareNativeModelDraw(gpu.device.Get(),std::move(input),native_state_reuse_disabled?nullptr:kept_base);}
            catch(const std::exception& error){static LONG64 failures{};if(InterlockedIncrement64(&failures)<=8){char row[400]{};
                sprintf_s(row,"{\"event\":\"native_model_draw_prepare_failed\",\"model\":\"%.60s\",\"engine_pass\":%u,\"recipe\":%u,\"what\":\"%.200s\"}\n",
                    NativeModelDataName(model.model).c_str(),prepared.engine_pass,static_cast<uint32_t>(prepared.recipe),error.what());Log(row);}continue;}
            if(draw->ReusedState())++chunk_outcomes[8];
            if(keep && !tess_row && draw->ReEncodable(kept_slots))chunk_kept_ops.push_back({kept_key,{draw,row_model_version,kept_serial,prepared.tiling,{*row_instances[0],*row_instances[1]},
                row_velocity_scale,row_snow},true});   // bound: no insertion past 16,384
            else if(keep && !tess_row)chunk_kept_ops.push_back({kept_key,{},false});   // review: a fresh draw that cannot be kept (legacy tables) drops the one it replaces
            {if(!velocity_draw && !forward_draw)paint_row(model,mesh,prepared,draw);   // D2
                chunk_frame->forward_early+=prepared.engine_pass==0x1F;
                if(forward_draw && !water_draw)out.forward_rows.push_back(row_index);
                if(fade_mark && forward_draw && !water_draw)chunk_frame->fade_mask_draws.push_back(draw.get());   // I-0140: forward_draws holds it
                (water_draw?(prepared.engine_pass==0x24?chunk_frame->water_depth_draws:chunk_frame->water_draws):velocity_draw?chunk_frame->velocity_draws:forward_draw?chunk_frame->forward_draws:chunk_frame->draws).push_back(std::move(draw));}asm_drawn=true;
            kprofile::SwitchIfActive(kprofile::PrepRowSetup);kprofile::AddIfActive(kprofile::PrepDraws);}
        else if(water_draw){static LONG64 water_failures{};if(InterlockedIncrement64(&water_failures)<=24){char row[320]{};   // WT-3/WT-4 witness
            sprintf_s(row,"{\"event\":\"native_water_draw_incomplete\",\"engine_pass\":%u,\"recipe\":%u,\"mesh\":%u,\"stage\":%u,\"resource\":\"%.80s\",\"register\":%u,\"input\":%u,\"missing\":[%u,%u,%u,%u,%u,%u]}\n",
                prepared.engine_pass,prepared.recipe,mesh.current.ordinal,failed_stage,failed_resource?failed_resource:"(interface)",failed_register,failed_input,
                missing_constants,missing_textures,unsupported_palettes,unsupported_layouts,unsupported_samplers,missing_interfaces);Log(row);}}
        else if(velocity_draw){static LONG64 velocity_failures{};if(InterlockedIncrement64(&velocity_failures)<=24){char row[256]{};sprintf_s(row,"{\"event\":\"native_velocity_draw_incomplete\",\"asm_part\":%d,\"mesh\":%u,\"stage\":%u,\"resource\":\"%.80s\",\"register\":%u}\n",
            model.asm_part,mesh.current.ordinal,failed_stage,failed_resource?failed_resource:"(interface)",failed_register);Log(row);}}
        QueryPerformanceCounter(&draw_end);native_prep_ticks[2]+=draw_end.QuadPart-draw_start.QuadPart;
    }};
    // I-0075 (I-0071 point 3c, ER: each draw prepared in the worker that records it, I-0073 section 3): the rows in chunks of
    // prepare_chunk_rows on the recording pool (free while the render worker prepares), each chunk with its own frame lists,
    // counters, shared-block table and deferred kept-draw changes, merged in row order; the model versions and the repeated
    // rows' occurrences are made first, on this thread, and the kept draws are only read while the chunks run. The menu views
    // and small scenes run as one chunk on this thread.
    const size_t row_count=scene->prepared_meshes.size();
    if(std::any_of(scene->prepared_meshes.begin(),scene->prepared_meshes.end(),[](const auto& p){return NativeTessellatedRecipe(p.recipe);}))make_tess_view();
    if(keep){std::unordered_map<NativeDryRowKey,uint32_t,NativeDryRowHash> seen;seen.reserve(row_count);
        for(size_t i=0;i<row_count;++i){const auto& p=scene->prepared_meshes[i];if(NativeTessellatedRecipe(p.recipe))continue;const auto& m=scene->models.at(p.model);
            occurrences[i]=seen[{m.entity,m.model,p.mesh,p.fade,p.engine_pass,p.recipe,0}]++;(void)model_version_compute(p.model);}}
    auto& prepare_pool=RecordingPool();
    const bool chunked=!offscreen && !native_prepare_chunks_disabled && prepare_pool.Workers()>1 && row_count>=2*prepare_chunk_rows;
    std::vector<PrepareChunk> chunks(chunked?(row_count+prepare_chunk_rows-1)/prepare_chunk_rows:1);
    if(!chunked)run_rows(chunks[0],0,row_count);
    else{std::atomic<size_t> ticket{};
        // Review M1: the measure-mode split counts the workers' rows too (the phase state is per thread): a worker opens the row
        // phase when the caller is measured; the phases' self times are then summed over the threads.
        const bool profiled=kprofile::state.current!=kprofile::None;
        const std::function<void(size_t)> job=[&](size_t worker){std::optional<kprofile::Scope> phase;if(profiled && worker)phase.emplace(kprofile::PrepRowSetup);
            for(size_t c;(c=ticket.fetch_add(1,std::memory_order_relaxed))<chunks.size();)
                run_rows(chunks[c],c*prepare_chunk_rows,(std::min)(row_count,(c+1)*prepare_chunk_rows));};
        prepare_pool.Run(job);native_prepare_chunked.fetch_add(1,std::memory_order_relaxed);native_prepare_chunks.fetch_add(chunks.size(),std::memory_order_relaxed);}
    for(auto& chunk:chunks){auto& part=*chunk.frame;
        const auto append=[](auto& to,auto& from){to.insert(to.end(),std::make_move_iterator(from.begin()),std::make_move_iterator(from.end()));};
        append(frame->draws,part.draws);append(frame->forward_draws,part.forward_draws);append(frame->velocity_draws,part.velocity_draws);
        append(frame->fade_mask_draws,part.fade_mask_draws);   // I-0140

        append(frame->water_depth_draws,part.water_depth_draws);append(frame->water_draws,part.water_draws);frame->forward_early+=part.forward_early;
        for(size_t k=0;k<frame->frame_blocks.size();++k)   // the first longest block of each kind, as one thread took it
            if(part.frame_blocks[k] && (!frame->frame_blocks[k] || frame->frame_blocks[k]->size()<part.frame_blocks[k]->size()))frame->frame_blocks[k]=part.frame_blocks[k];
        all_missing_constants|=chunk.missing_constants;all_missing_textures+=chunk.missing_textures;all_unsupported_palettes+=chunk.unsupported_palettes;
        all_unsupported_layouts+=chunk.unsupported_layouts;all_unsupported_samplers+=chunk.unsupported_samplers;all_missing_interfaces+=chunk.missing_interfaces;
        all_unbounded_dynamic+=chunk.unbounded_dynamic;
        const auto add=[](NativeAsmOutcome& to,const NativeAsmOutcome& from){to.prepared+=from.prepared;to.drawn+=from.drawn;for(size_t i=0;i<to.fail.size();++i)to.fail[i]+=from.fail[i];};
        for(const auto& [part_key,outcome]:chunk.asm_outcomes)add(all_asm_outcomes[part_key],outcome);
        for(const auto& [family,outcome]:chunk.family_outcomes)add(all_family_outcomes[family],outcome);
        for(size_t k=0;k<chunk.outcomes.size();++k)kept.outcomes[k]+=chunk.outcomes[k];
        for(auto& op:chunk.kept_ops){   // bound: no insertion past native_kept_draw_limit; a fresh draw that cannot be kept drops the one it replaces
            if(op.insert && (kept.draws.size()<native_kept_draw_limit || kept.draws.contains(op.key)))kept.draws[op.key]=std::move(op.entry);
            else kept.draws.erase(op.key);}
        for(const auto& [index,draw]:chunk.paints){const auto& p=scene->prepared_meshes[index];const auto& m=scene->models.at(p.model);apply_paint_row(m,m.meshes.at(p.mesh),p,draw);}}
    // I-0113 tranche 15: the forward rows in 57A20's order. Each draw pushes its row (both lists in the same places), so a count that
    // differs is an invariant broken: logged, the rows then drawn in the prepared order (0x1F first, forward_early from the chunks).
    size_t forward_unboxed=0;
    {std::vector<size_t> rows;rows.reserve(frame->forward_draws.size());for(const auto& chunk:chunks)rows.insert(rows.end(),chunk.forward_rows.begin(),chunk.forward_rows.end());
        if(rows.size()==frame->forward_draws.size())forward_unboxed=SortNativeForwardRows(*scene,*frame,rows);
        else{static LONG64 unsorted_rows{};if(InterlockedIncrement64(&unsorted_rows)<=4)Log("{\"event\":\"native_forward_rows_unsorted\"}\n");}}
    if(popin){std::vector<std::pair<size_t,bool>> rows;for(auto& chunk:chunks)rows.insert(rows.end(),chunk.popin_rows.begin(),chunk.popin_rows.end());PopinNativeFrame(*scene,rows);
        PopinLoadFrame(*scene,presentation_serial,frame->draws.size(),all_missing_textures,all_missing_constants,rows);}
    kprofile::SwitchIfActive(kprofile::PrepFrameTail);
    // C4-2: kept draws unused for native_kept_draw_unused prepares and model records unseen as long are dropped; outcomes
    // every 600th prepare (hits, new rows, model changed, row changed, epoch drops, the two former shadow slots, frame refusals).
    // I-0070 (camera turns, Archdragon Peak): at 8 prepares a turn of the camera rebuilt every row coming back into view (the
    // prepare 43-55 ms for three frames, run i70-native-11-m32-sweep-i70b); at 600 (10 s at 60 frames/s) they are reused.
    // Bound: past native_kept_draw_bound, the draws unused longest go first (the same run: about 23 global descriptors a draw,
    // 14 chunks of the global heap's 64 for 5,000 draws).
    if(keep){
        if(kept_serial%8==0){std::erase_if(kept.draws,[&](const auto& entry){return kept_serial-entry.second.last_used>=native_kept_draw_unused;});
            std::erase_if(kept.models,[&](const auto& entry){return kept_serial-entry.second.last_seen>=native_kept_draw_unused;});}
        if(kept.draws.size()>native_kept_draw_bound){std::vector<std::pair<uint64_t,NativeDryRowKey>> unused;
            for(const auto& [key,entry]:kept.draws)if(entry.last_used!=kept_serial)unused.emplace_back(entry.last_used,key);
            const auto excess=(std::min)(kept.draws.size()-native_kept_draw_bound,unused.size());
            std::partial_sort(unused.begin(),unused.begin()+excess,unused.end(),[](const auto& a,const auto& b){return a.first<b.first;});
            for(size_t i=0;i<excess;++i)kept.draws.erase(unused[i].second);}
        if(kept_serial<=3 || kept_serial%600==0){const auto& o=kept.outcomes;char row[400]{};sprintf_s(row,
            "{\"event\":\"native_persistent_draws\",\"prepares\":%llu,\"draws\":%zu,\"models\":%zu,\"hits\":%llu,\"new\":%llu,\"model_changed\":%llu,\"row_changed\":%llu,\"epoch_drops\":%llu,\"shadow_checks\":%llu,\"mismatches\":%llu,\"frame_refusals\":%llu,\"state_reused\":%llu}\n",
            static_cast<unsigned long long>(kept_serial),kept.draws.size(),kept.models.size(),static_cast<unsigned long long>(o[0]),static_cast<unsigned long long>(o[1]),
            static_cast<unsigned long long>(o[2]),static_cast<unsigned long long>(o[3]),static_cast<unsigned long long>(o[4]),static_cast<unsigned long long>(o[5]),
            static_cast<unsigned long long>(o[6]),static_cast<unsigned long long>(o[7]),static_cast<unsigned long long>(o[8]));
            // The model versions that moved, by the first differing input.
            std::string line(row);line.resize(line.size()-2);line+=",\"model_changes\":{";bool first=true;
            for(size_t k=0;k<kept.model_changes.size();++k)if(kept.model_changes[k]){line+=(first?"\"":",\"")+std::string(NativeKeptModelFields[k])+"\":"+std::to_string(kept.model_changes[k]);first=false;}
            line+="}}\n";Log(line.c_str());kept.outcomes={};kept.model_changes={};}
        kept_hold.unlock();}
    static LONG64 attempts{};
    static LONG64 offscreen_attempts{};   // review 1180: menu views are prepared every frame; their rows are bounded too
    {const auto attempt=offscreen?0:InterlockedIncrement64(&attempts);const auto offscreen_attempt=offscreen?InterlockedIncrement64(&offscreen_attempts):0;
     if((!offscreen && (attempt<=8 || attempt%600==0)) || (offscreen && (offscreen_attempt<=12 || offscreen_attempt%600==0))){char row[512]{};sprintf_s(row,
        "{\"event\":\"native_selected_draw_preparation\",\"view_serial\":%llu,\"prepared_rows\":%zu,\"draws\":%zu,\"missing_constant_mask\":%u,\"missing_textures\":%u,\"unsupported_palettes\":%u,\"unsupported_layouts\":%u,\"unsupported_samplers\":%u,\"missing_interfaces\":%u,\"unbounded_dynamic\":%u,\"aa_policy\":\"native_ordinary_default\"}\n",
        scene->serial,scene->prepared_meshes.size(),frame->draws.size(),all_missing_constants,all_missing_textures,all_unsupported_palettes,all_unsupported_layouts,
        all_unsupported_samplers,all_missing_interfaces,all_unbounded_dynamic);Log(row);}}
    LogNativeAsmSummary(*scene,all_asm_outcomes,all_family_outcomes);
    // I-0141 tranche 20: a main view with forward draws and no G-buffer draw (its G-buffer rows absent, or all refused at their
    // preparation) is lit and drawn too, as the original draws its frames: 41000060's cut 2 (the eclipse over the clouds) kept 5 forward draws and no G-buffer one and was dropped, its frames
    // cleared (I-0138 tranche 10). The menu views keep the G-buffer rule (RenderView requires their G-buffer rows).
    const bool forward_only=!offscreen && frame->draws.empty() && !frame->forward_draws.empty();
    if((!frame->draws.empty() || forward_only) && scene->lighting)frame->lighting=scene->lighting;   // cP7: built at collect (ProduceNativeSceneLighting)
    // DECALS-WAVE1 D1: the lit main view only; not on a forward-only frame (its G-buffer is cleared: no decal pixel, the step review's L3)
    if(!offscreen && frame->lighting && !frame->draws.empty())BuildNativeDecalDraws(scene,*frame);
    // The forward pass draws into the lit image: without lighting it is withheld.
    if(!frame->lighting){frame->fade_mask_draws.clear();frame->forward_draws.clear();frame->forward_keys.clear();frame->forward_sorted=0;frame->forward_1f=0;frame->forward_21=0;frame->water_depth_draws.clear();frame->water_draws.clear();}
    // WT-2: the readable depth the water rows sample (their t56 binding), the copy's destination; I-0105: or a forward row's
    // (the feathered effect models), copied at the start of the forward pass when there is no water.
    for(const auto* list:{&frame->water_depth_draws,&frame->water_draws,&frame->forward_draws}){
        for(const auto& draw:*list){for(const auto& srv:draw->Input().srvs)
            if(srv.stage==ds3content::ShaderStage::Pixel && srv.shader_register==56 && srv.resource &&
               srv.resource->GetDesc().Format==DXGI_FORMAT_R32G8X24_TYPELESS){frame->water_depth_copy=srv.resource;break;}   // step review: the readable depth only, never a material's t56
            if(frame->water_depth_copy)break;}
        if(frame->water_depth_copy)break;}
    if(!frame->water_depth_copy){frame->water_depth_draws.clear();frame->water_draws.clear();}
    if(!frame->water_draws.empty() || !frame->water_depth_draws.empty()){static LONG64 rows{};if(const auto n=InterlockedIncrement64(&rows);n<=8 || n%600==0){char row[200]{};
        sprintf_s(row,"{\"event\":\"native_water_draws\",\"n\":%lld,\"depth\":%zu,\"surface\":%zu,\"forward_early\":%zu}\n",n,frame->water_depth_draws.size(),frame->water_draws.size(),frame->forward_early);Log(row);}}
    {static LONG64 forward_samples{};if(!frame->forward_draws.empty() && [&]{const auto n=InterlockedIncrement64(&forward_samples);return n<=4 || n%600==0;}()){
        char row[200]{};sprintf_s(row,"{\"event\":\"native_forward_draws\",\"draws\":%zu,\"deferred_draws\":%zu,\"rows_0x29\":%zu,\"unboxed\":%zu}\n",frame->forward_draws.size(),frame->draws.size(),
            frame->forward_draws.size()-std::min(frame->forward_sorted,frame->forward_draws.size()),forward_unboxed);Log(row);}}
    // C1 witness: prepared rows by engine pass and by owner kind (asm part, 1000 SprjModelDrawEntity, 1001 skinned map
    // object, other map pieces), with the unions per kind, every 60th main-view frame.
    static const bool pass_rows_witness=ModuleFlag(L"light-observe.flag");   // I-0120 B7: the witness only with light-observe.flag
    if(pass_rows_witness && !offscreen){static LONG64 row_samples{};if(const auto n=InterlockedIncrement64(&row_samples);n%60==1){
        std::map<std::pair<uint32_t,int>,uint32_t> rows;
        for(const auto& p:scene->prepared_meshes){const auto part=scene->models[p.model].asm_part;
            ++rows[{p.engine_pass,part<0?-1:part>=1000?part:0}];}
        std::string cells;for(const auto& [key,count]:rows){char cell[48]{};sprintf_s(cell,"%s[%u,%d,%u]",cells.empty()?"":",",key.first,key.second,count);cells+=cell;}
        std::map<int,std::array<uint32_t,3>> unions;   // per owner kind: models, with class 0x100, with flags bit 15
        for(const auto& m:scene->models){auto& u=unions[m.asm_part<0?-1:m.asm_part>=1000?m.asm_part:0];++u[0];
            if(m.material_mask && (*m.material_mask&0x100))++u[1];
            if(m.material_flags && (*m.material_flags&0x8000))++u[2];}
        std::string kinds;for(const auto& [kind,u]:unions){char cell[64]{};sprintf_s(cell,"%s[%d,%u,%u,%u]",kinds.empty()?"":",",kind,u[0],u[1],u[2]);kinds+=cell;}
        const auto& pose=scene->camera.camera_pose;char eye[160]{};sprintf_s(eye,"[%.2f,%.2f,%.2f],\"axis_z\":[%.3f,%.3f,%.3f]",pose[12],pose[13],pose[14],pose[8],pose[9],pose[10]);
        const std::string row="{\"event\":\"native_pass_rows\",\"frame\":"+std::to_string(n)+",\"eye\":"+eye+",\"rows\":["+cells+"],\"unions\":["+kinds+"]}\n";Log(row.c_str());}}
    // 1186 velocity (main view only): the target is cleared and receives the Velo rows; the native motion-vector resolve
    // (upscaler preparation, I-0068 VELOCITY-MOTIONBLUR §6) reads it with the scene depth.
    if(scene->glow && !offscreen)frame->glow=std::make_shared<const NativeGlowInput>(*scene->glow);   // W5d GL-3
    if(scene->effects && !offscreen)frame->effects=scene->effects;   // FFX-DRAW N6
    // W10 TM-2 (on by default, no-yebis-tonemap.flag opts out; with the YEBIS feed): our tone-map pass from the blended Tone Map record (words 0 Exposure,
    // 1 Gamma, 2 ToneType, 3 ToneMappingFactor (the Reinhard curve's), 4 FilmicContrast, 5-7 ColorH/S/V; TM-3: 26 Vignette Power, 27 VignetteFovDependence); refused with a row
    // (the sRGB composition stays) when the record asks for what the pass does not do yet: auto exposure, a ToneType other than 2 and 3
    // (3, the Reinhard curve: I-0138 part 4c)
    // (I-0113 tranche 20: ColorV, Contrust (entry 8), the fov vignette and the exposure and gamma guards served, yebis_tonemap.h;
    // hue, value, contrast, exposure and gamma refused only without the executable's colour data). A positive Vignette Power selects the vignette
    // variant (inferred from m31 0.2: vignette, m37 0: none) with its TEXCOORD2 for the camera's aspect. A row at each change of that
    // state or of the words read.
    // I-0138 tranche 1 (ROADMAP workstream 11): the pass is our own (PostToneStage: our programmes and parameter block, the same
    // bytes as the original's draws). Tranche 8: YEBIS's programmes and its two test paths (post-tone-old.flag, post-tone-verify.flag)
    // are gone from the module; the offline tests keep the library's programmes as their oracle (post_tone_stage_test).
    {static const bool tonemap_flag=NativeYebisPass(L"tonemap");
        if(tonemap_flag && !offscreen && scene->yebis && scene->yebis->complete){const auto& tone=scene->yebis->tone;
            const auto post=NativePostExtent(scene->camera);   // I-0111: the vignette's aspect the output's
            const auto word=[&](size_t i){float f;std::memcpy(&f,&tone[i],4);return f;};
            ds3native::yebis::TonemapInputs in;in.exposure=word(0);in.gamma=word(1);in.filmic_contrast=word(4);in.color_h=word(5);in.color_s=word(6);in.color_v=word(7);in.contrast=word(8);
            in.tone_type=tone[2];in.mapping_factor=word(3);   // I-0138 part 4c: tone type 3 the Reinhard curve, its ToneMappingFactor
            // I-0132 R12g: the vignette the player's (the settings' vignette, live): Off draws the tone map without it, and the glare's
            // bright pass follows (its vignette the tone map's, GL-4 below).
            const float vignette_power=word(26),vignette_fov=word(27);const bool vignette=0.0f<vignette_power && NativeVignetteWanted();
            const auto& data=TonemapImageData();
            // I-0113 tranche 20: the fov term, the glare object's +0x138 = tan(fov / 2) of E69A0's camera record +0x50 (the vertical fov;
            // FUN_141753790 from the library's S+0x570 every frame, tan(pi/8) when it is not positive; the shafts' word, W10 LS-2)
            const float tan_half_fov=ds3native::yebis::VignetteFovTerm(data.vignette,scene->camera.vertical_fov);const auto t=data.ok?ds3native::yebis::BuildTonemap(data.data,in):ds3native::yebis::TonemapConstants{};
            const char* refused=!data.ok?"executable data":scene->yebis->auto_exposure?"auto exposure":tone[2]!=2u && tone[2]!=3u?"tone type":!t.matrix_supported?"colour matrix":
                !t.guarded?"exposure or gamma":vignette && !(data.vignette.tan_eighth_pi>0.0f && post[0] && post[1] && (vignette_fov==0.0f || std::isfinite(tan_half_fov)))?"vignette data":nullptr;
            std::array<float,4> rect{};if(vignette && !refused)rect=ds3native::yebis::VignetteRect(data.vignette,vignette_power,float(post[0])/float(post[1]),vignette_fov,vignette_fov!=0.0f?tan_half_fov:0.0f);   // the render aspect +0x328 taken as the output's (checked at 1280x720 only)
            {static std::string last="unset";const std::string now=std::string(refused?refused:"")+NativeWordsList(&tone[0],9)+NativeWordsList(&tone[26],2)+(vignette?"v":"-");
                if(now!=last){last=now;char row[768]{};
                sprintf_s(row,"{\"event\":\"native_yebis_tonemap\",\"programmes\":\"%s\",\"on\":%s,\"vignette_drawn\":%s,\"refused\":\"%s\",\"exposure\":%g,\"gamma\":%g,\"tone_type\":%u,\"contrast\":%g,\"hsv\":[%g,%g,%g],\"contrust\":%g,\"exposure_set\":%g,\"curve\":[%.9g,%.9g,%.9g],\"matrix_840\":\"%08X\",\"vignette\":[%g,%g],\"rect\":[%.9g,%.9g]}\n",
                    "ours",refused?"false":"true",vignette && !refused?"true":"false",refused?refused:"",in.exposure,in.gamma,tone[2],in.filmic_contrast,in.color_h,in.color_s,in.color_v,in.contrast,t.exposure,t.curve[0],t.curve[1],t.curve[2],
                    [&]{uint32_t u;std::memcpy(&u,&t.color_matrix[0],4);return u;}(),vignette_power,vignette_fov,rect[2],rect[3]);Log(row);}}
            if(!refused){auto input=std::make_shared<NativeTonemapInput>();
                input->dither=data.dither;input->exposure=0.0f<in.exposure?t.exposure:1.0f;   // DLSS's exposure: 1 where the guard put FLT_MIN (a black scene term)
                if(vignette){input->vignette=true;input->vignette_uv=ds3native::yebis::VignetteCoordinates(rect);}
                input->post=ds3native::BuildPostTone(t,vignette?&input->vignette_uv:nullptr);
                // I-0138 tranche 6: a candidate look asked by a test flag or the settings (04 NativeToneLookRequested).
                input->post_record=input->post;
                {const auto asked=NativeToneLookRequested(presentation_serial);
                    const bool sized=post[0] && post[1];input->post.hdr_knee=asked.hdr_knee;
                    input->look_neutral=ds3native::ApplyToneLook(input->post,asked.look,asked.neutral,asked.aberration);
                    // I-0138 tranche 7: the bloom's strength (the settings' row, percent of DS3's own): the glare's weight in this pass,
                    // the light shafts with it (they ride the glare's picture).
                    input->post.glare_scale*=float((std::min)(NativeSettingsCurrent().bloom,ds3native::settings::kStrengthMost))/100.0f;
                    // The film grain (the settings' row, none by default), moved every frame.
                    ds3native::SetFilmGrain(input->post,NativeSettingsCurrent().grain,sized?post[1]:0u,uint32_t(presentation_serial));}
                // I-0132 R11h: the automatic exposure with the game's one target, on by default at half strength (the goal half the solved
                // log2 factor: the dark views stay darker; Thomas, 2026-11-09 23:55, on R11h's sheets: « Je suis ta recommandation pour 1 et
                // 2 »); test tools: no-auto-exposure.flag (the area's fixed exposure, as before), rt-auto-exposure-full.flag (full strength, under the same lower bound).
                // I-0138 tranche 5e (Thomas, 2026-11-14 22:16, D-0280): it brightens the dark views and never darkens under the area's own
                // exposure (the factor's lower bound 1, log2 0): a zone the game makes bright on purpose keeps its level (Archdragon Peak,
                // the King of the Storm's arena: one stop to a stop and three quarters under the original's before, I-0141's runs).
                {static const bool off=ModuleFlag(L"no-auto-exposure.flag"),full=ModuleFlag(L"rt-auto-exposure-full.flag");
                    if(!off){input->auto_exposure=true;input->auto_params.target=NativeAutoExposureTarget();input->auto_params.strength=full?1.0f:0.5f;
                        input->auto_params.min_log2=ds3native::kAutoExposureMinLog2;}}   // I-0138 tranche 3's test flag for bounds read from Elden Ring removed (Thomas, 2026-11-13: the half strength stays)
                frame->tonemap=std::move(input);}}}
    // W10 GR (on by default; with the tone-map input): the colour grading natively (W10-NATIVE-PASSES 12). The original draws it
    // where the blended ColorGrading record's word 0 (144962228, the LUT-copy trigger) is set and a LUT texture is found
    // (FUN_1400E69A0: the 3D LUT, the record's flag and a texture at +0x358 or +0x360): the texture the record named
    // "<prefix>%04d" with word 1 (14021CC20), the prefix "mAA_BB_CGrading" of the record's map file (14017B270: the gparam name
    // and "CGrading"; observed "m31_00_CGrading" at m31, Reference1696). No texture of that name (the DLC maps ship no cgrading
    // TPF, m37's word 1 is -1): no draw, as the original. I-0138 tranche 6 (the prefix read: FUN_140D28490, called for every parsed
    // file by CSGparamResCap's parse 140D272C0 through FUN_140095100, which stores it at the file entry's +0x350; 14017B270 appends
    // « CGrading »): the prefix is "m%02d_%02d_" from the AA and BB of the file's own name for a map file (kind 7, mAA_BB_VVVV) and for
    // a cutscene file (kind 0xB, sAA_BB_VVVV: the map's strip of that id, not a cutscene's own), and empty for every other file
    // (mapbase, default, remobase: "CGrading%04d", a name no shipped texture carries, INFERRED: no draw). Review:
    // a strip other than 16x256 R8G8B8A8_UNORM (FUN_1400E5110 tests the size only and falls back to the library's default LUT; no
    // shipped strip differs) is refused: both departures are logged. A row at each change of the outcome.
    {static const bool grading_flag=NativeYebisPass(L"grading");
        if(grading_flag && frame->tonemap && !frame->tonemap->look_neutral && scene->yebis && scene->yebis->grading_ready){const auto& g=scene->yebis->grading;const uint32_t area=scene->yebis->grading_area;   // look_neutral: I-0138 tranche 6, a candidate without the area's LUT
            char name[64]{};const char* refused=nullptr;std::shared_ptr<const bootstrap::NativeTextureRecord> record;
            // The LUT's name from the record's file (world/post_tone.h GradingLutName, K-0495).
            const auto& source=scene->yebis->grading_source;
            if(!g[0])refused="off";else if(static_cast<int32_t>(g[1])<0)refused="no LUT id";
            else if(!ds3native::GradingLutName(source,area,static_cast<int32_t>(g[1]),name))refused="non-map record";
            if(!refused){
                // Review: acquired under the lock RetireTexture takes to drop the name, so the record is the named texture's.
                bool named=false;{const std::lock_guard hold(native_cube_name_lock);const auto found=native_grading_names.find(name);
                    if(found!=native_grading_names.end()){named=true;record=textures.Acquire(found->second);}}
                if(!named)refused="texture absent";
                else if(!record || !record->resident || !record->resource)refused="texture not resident";
                else if(record->description.width!=16 || record->description.height!=256 || record->description.format!=DXGI_FORMAT_R8G8B8A8_UNORM)refused="texture not a 16x256 RGBA8 strip";}
            // I-0138 tranche 8: the strip's copy into the volume is our own compute (post_tone.hlsl cs_lut_copy, GX's LutCopy2DTo3DCs
            // no longer read), so the pass needs no programme from the engine: the default path's refusal « programmes » (the copy not
            // yet in the engine's compute repository, or a pin mismatch) is gone with it (the step review's L2).
            {static std::string last="unset";const std::string now=std::string(refused?refused:"")+name+NativeWordsList(g.data(),2)+scene->yebis->grading_source;
                if(now!=last){last=now;char row[256]{};sprintf_s(row,"{\"event\":\"native_yebis_grading\",\"on\":%s,\"refused\":\"%s\",\"name\":\"%s\",\"words\":[%s],\"area\":%u,\"source\":\"%.24s\"}\n",
                    refused?"false":"true",refused?refused:"",name,NativeWordsList(g.data(),g.size()).c_str(),area,scene->yebis->grading_source.c_str());Log(row);}}
            if(!refused){auto input=std::make_shared<NativeGradingInput>();input->lut=record->resource.Get();input->lut_format=record->description.format;
                input->lut_generation=record->generation;input->lut_owner=record;frame->yebis_grading=std::move(input);}}}
    // W10 GL-3 (on by default; with the tone-map input): the glare family's plan from the library's globals block 144961F70 the
    // native feed's converters wrote this frame (GlareQuality, exposure, GlareLuminance, GlareThreshold, GlareNumLevels, radius
    // scale, luminance gamma, RemapFactor, sampling threshold) and the lit HDR's extent. F (CRenderGlare +0x114) is 64 and the
    // GlareShape 1 record (1, 1, 0) as Reference1649 (m37) and Reference1650 (m31) show (INFERRED constants: the library is not
    // running natively); another shape: tranche 8c's answer, below (the bloom gamma +0x1B0 is the library's 1: below). A row at
    // each change of the outcome or of the words read.
    {static const bool glare_flag=NativeYebisPass(L"glare");
        if(glare_flag && frame->tonemap && scene->camera.width && scene->camera.height){std::array<uint8_t,0x370> block{};
            const auto& data=GlareImageData();const bool read=scene->yebis && scene->yebis->globals_read;if(read)block=scene->yebis->globals;   // Fable review: the collect's snapshot
            const auto word=[&](size_t at){float f;std::memcpy(&f,block.data()+at,4);return f;};const auto integer=[&](size_t at){int32_t i;std::memcpy(&i,block.data()+at,4);return i;};
            const auto post=NativePostExtent(scene->camera);   // I-0111: the glare at the output extent (DLSS's output in a scaled mode)
            ds3native::yebis::GlareInputs in;in.width=post[0];in.height=post[1];in.quality=integer(0x48);in.exposure=word(0x98);
            in.luminance=word(0xDC);in.threshold=word(0xE0);in.levels=integer(0xE4);in.radius_scale=word(0xF8);in.composite_gamma=word(0xFC);
            // I-0138 tranche 8: the bloom gamma is CRenderGlare +0x1B0, copied each frame from CPostEffect +0xC2C (EndPostEffectScene,
            // 1416E7887..1416E7896), whose only store found is the library's init (1416F0F67: 1.0; a block copy not excluded); READ, GhidraMCP 2026-11-16 (a ghidra-research
            // reading, its call sites re-read by the pilot). The globals word +0x100 this line read before is another field (E69A0 passes it to
            // FUN_1416CB820, CRenderGlare +0x1A8, GlareGhostConcentricDistortion): 0 at the Twin Princes' arena and 0.79 to 0.99 in its cutscene, where the glare was refused for it.
            in.bloom_gamma=1.0f;
            in.remap=word(0x244);in.sampling_threshold=word(0x304);in.factor=64.0f;in.shape_bloom=1.0f;in.shape_scale=1.0f;in.shape_tint=0.0f;
            // GL-4: the bright pass's vignette follows the tone map's (CRenderGlare +0x160 where Vignette Power is positive), with its
            // coordinates (the captured m50 bright-pass vertices hold the tone-map draw's TEXCOORD2 words).
            in.vignette=frame->tonemap->vignette;in.vignette_uv=frame->tonemap->vignette_uv;
            // I-0138 tranche 8f: GlareShape 3 (« Standard », 32000020's glare): the shape word, its record from the executable's tables
            // (yebis::ShapeRecord), the streaks' and the ghost's globals (GlareStarFilterThreshold +0xEC, GlareStarLengthFovDependence +0xF0,
            // GlareStarSoftness +0xF4, GlareGhostConcentricDistortion +0x100), and the object's fields from their native sources (READ,
            // evidence/I-0138/t8f-readings/glare-ev-base-reading.md; yebis_glare_test checks +0x15C, +0x2F8 and +0x158 against the original's captured objects, +0x138 = tan(fov / 2) yebis_shafts_test against the captures, VignetteFovTerm's edge cases yebis_tonemap_test):
            // +0x138 tan(fov / 2) of the frame's camera (the shafts' and the vignette's), +0x15C the DoF record's diaphragm rotation
            // (+0x1C4, +0x1C8, +0x1CC), +0x2F8 the base level from the base width, +0x158 the globals' word +4 (1/60 by construction).
            in.shape=integer(0xD8);
            if(in.shape==3){in.record=ds3native::yebis::ShapeRecord(data.data);in.star_threshold=word(0xEC);in.star_fov=word(0xF0);in.star_softness=word(0xF4);
                in.ghost_distortion=word(0x100);in.tan_half=ds3native::yebis::VignetteFovTerm(TonemapImageData().vignette,scene->camera.vertical_fov);
                in.ev=ds3native::yebis::GlareRotation(in.tan_half,integer(0x1CC),word(0x1C4),word(0x1C8));
                in.base_level=ds3native::yebis::GlareBaseLevel(ds3native::yebis::GlareBaseSize(data.data,in.width,in.height,in.quality)[0],in.width);
                in.frame_time=word(0x4);}
            // The plan is rebuilt only when the words it reads or the extent change (the frame inputs share it otherwise).
            static std::mutex cache_lock;static std::string cached_key;static std::shared_ptr<const NativeGlareInput> cached;const std::lock_guard hold_cache(cache_lock);
            // I-0141 tranche 26 (test tooling): the runner's share variant serves the family empty, as the library's own skip below.
            const bool share_off=(witness_share_variant.load(std::memory_order_relaxed)&kShareNoGlare)!=0;
            const std::string key=std::string(reinterpret_cast<const char*>(block.data()),block.size())+std::to_string(in.width)+"x"+std::to_string(in.height)+
                std::string(reinterpret_cast<const char*>(&in.vignette_uv),sizeof(in.vignette_uv))+(in.vignette?"v":"")+
                std::string(reinterpret_cast<const char*>(&in.tan_half),sizeof(in.tan_half))+(share_off?"g":"");   // tranche 8f: shape 3 follows the camera's fov; I-0141 tranche 26: the share variant
            // LS-3: with the shafts (on by default, no-yebis-shafts.flag) the plan follows the camera and is built every frame (the stage relays out only when
            // the draws' shape changes).
            // I-0138 tranche 7: and the settings' Light shafts row (live).
            static const bool shafts_test=NativeYebisPass(L"shafts");const bool shafts_flag=shafts_test && NativeSettingsCurrent().shafts;
            // I-0138 tranche 8c (the final review's M1): the motion blur's own Alpha_R when the globals ask for the blur (its Enable +0x1DC,
            // mode 2 +0x1E0, the one the native serves: the library's +0xC64 is set at a mode above 1 or by the falloff test, one of the stages that make it draw Alpha_R): it follows the camera too, so the plan is
            // then built every frame.
            static const bool motionblur_test=NativeYebisPass(L"motionblur");const bool blur_alpha=motionblur_test && read && integer(0x1DC) && integer(0x1E0)==2;
            const bool every_frame=shafts_flag || blur_alpha;
            if(!every_frame && key==cached_key){if(cached)frame->yebis_glare=cached;}else{cached.reset();cached_key=every_frame?std::string():key;   // a refusal is kept too
            ds3native::yebis::GlarePlan plan;
            // I-0138 tranche 8c: the library's answer to the shape (READ, GhidraMCP 2026-11-21, a ghidra-research reading, its gate re-read by
            // the pilot): EndPostEffectScene_InternalProcess skips the whole glare family (bright pass, pyramid, streaks, star, ghost,
            // afterimage, light shafts, composite) when the stored shape (CPostEffect +0xBC4, from this word through 1416C9690) is 0
            // (1416E7583) or the glare luminance is not positive (1416E758B), and its tone map then takes no glare (GatherToneMapFlags'
            // factor 0); the depth of field (1416E631E) and the motion blur (1416E63BE) run before that gate and never read the shape.
            // 1416C9690 stores this word as it is each frame (READ); the luminance is stored by 1416C9670 only while RemapFactor is
            // positive (READ), the frame's word read here (INFERRED alike: no glare row with RemapFactor not positive in the 833 journals holding
            // glare rows of the last ten days, evidence/I-0066/artifacts). So the glare family's own skip no longer takes the depth of field away: the plan is served empty at the
            // frame's extent (no draw, no shaft, the tone map's t1 black). The motion blur keeps its
            // Alpha_R there (AppendNativeAlphaR below: the library draws it for the blur whatever the shape and, since tranche 8d,
            // whatever the Dof record's Enable, in the depth spaces of NativeAlphaRInputs). Tranche 8f: shape 3 (« Standard »: the
            // bloom with four-arm streaks, a ghost and an afterimage) is served, its shafts since tranche 8g; the others the library draws are not
            // served natively (named in the row): no bloom and no shaft there, their DoF and motion blur kept the same way.
            const int shape=integer(0xD8);std::string shape_refused,skipped;
            if(!data.ok)plan.refused="executable data";else if(!read)plan.refused="globals";
            else if(const auto gate=ds3native::yebis::GlareShapeGate(shape,in.luminance);gate!=ds3native::yebis::GlareGate::Draw){
                using G=ds3native::yebis::GlareGate;
                if(gate==G::SkipLuminance)skipped="luminance";else if(gate==G::SkipShape0)skipped="shape 0";else shape_refused="glare shape "+std::to_string(shape);
                plan.width=in.width;plan.height=in.height;}
            else if(share_off){skipped="share variant";plan.width=in.width;plan.height=in.height;}   // I-0141 tranche 26
            // Tranche 8f (its step review's M2): the bright pass's mode 0 (a threshold of 0 at RemapFactor 1, READ,
            // evidence/I-0138/t8f-readings/bright-pass-modes.md; mode 0 not written) serves the family
            // empty, as a shape not served, so the frame keeps its depth of field and its blur (BuildGlare refuses it too).
            else if(in.remap==1.0f && in.threshold==0.0f){shape_refused="bright pass mode 0";plan.width=in.width;plan.height=in.height;}
            else{plan=ds3native::yebis::BuildGlare(data.data,in);
                // Tranche 8f (its final review's M1): any plan the builder refuses (for example a composite wider than 8 inputs at a very
                // large output, a streak count out of range, levels or a quality out of range) is served empty, named, as a shape not
                // served: the frame keeps its DoF and its blur. The row keeps the refused plan's base, levels and luminance.
                if(!plan.refused.empty()){shape_refused=plan.refused;ds3native::yebis::GlarePlan empty;empty.width=in.width;empty.height=in.height;
                    empty.base_width=plan.base_width;empty.base_height=plan.base_height;empty.levels=plan.levels;empty.luminance=plan.luminance;plan=std::move(empty);}}
            std::string shafts_refused="off";std::shared_ptr<const std::vector<std::vector<uint8_t>>> film_grain;
            // I-0138 tranche 8g: on shape 3 the shafts are the layers' composite's last layer (AppendShafts, MadT{n+1}: MadT5 when the streaks, the ghost and the afterimage all draw; m32_00_0100 holds shape 3
            // in play with its shafts on, CENSUS-WORLD), their factor the shape record's bloom (+8).
            if(shafts_flag && plan.refused.empty() && !plan.draws.empty())shafts_refused=AppendNativeShafts(plan,block,*scene,film_grain,in.shape==3?in.record.bloom:in.shape_bloom,presentation_serial);
            else if(shafts_flag)shafts_refused=!plan.refused.empty()?plan.refused:!shape_refused.empty()?shape_refused:"glare skipped: "+skipped;
            if(shafts_flag){static std::string last="unset";const std::string now=shafts_refused+"/"+std::to_string(plan.pre.size())+"/"+std::to_string(plan.shafts);
                if(now!=last){last=now;char row[256]{};sprintf_s(row,"{\"event\":\"native_yebis_shafts\",\"refused\":\"%.60s\",\"alpha\":%zu,\"shafts\":%d,\"draws\":%zu}\n",
                    shafts_refused.c_str(),plan.pre.size(),plan.shafts,plan.draws.size());Log(row);}}
            // After the shafts' row (step review L4: its « alpha » counts the shafts' own Alpha_R). Its refusals leave plan.pre empty: another lens is the blur's own « camera lens » (tested first in its row); without the executable's DoF words (DofImageData's ok, fixed for the process) the blur's row reads « no Alpha_R ».
            if(blur_alpha && plan.refused.empty() && plan.pre.empty())static_cast<void>(AppendNativeAlphaR(plan,block,*scene));
            // I-0138 tranche 2 (ROADMAP workstream 11): the family's programmes are our own (ChooseNativePostProgrammes: every
            // technique of the plan must be one of them, else the plan is refused and the row names the technique). Tranche 8: the
            // library's programmes and the two test flags that drew them are gone (the row keeps "programmes" "ours", "verify" false).
            std::shared_ptr<const GlarePrograms> programs;
            if(plan.refused.empty()){auto chosen=ChooseNativePostProgrammes(plan);programs=std::move(chosen.programs);
                if(!programs)plan.refused=std::move(chosen.refused);}
            {static std::string last="unset";const std::string now=plan.refused+shape_refused+skipped+NativeWordsList(reinterpret_cast<const uint32_t*>(block.data()+0x48),1)+
                    NativeWordsList(reinterpret_cast<const uint32_t*>(block.data()+0xD8),11)+NativeWordsList(reinterpret_cast<const uint32_t*>(block.data()+0x244),1)+
                    NativeWordsList(reinterpret_cast<const uint32_t*>(block.data()+0x304),1)+std::to_string(in.width)+"x"+std::to_string(in.height);
                if(now!=last){last=now;char row[640]{};sprintf_s(row,"{\"event\":\"native_yebis_glare\",\"programmes\":\"%s\",\"verify\":%s,\"on\":%s,\"refused\":\"%.60s\",\"skipped\":\"%s\",\"shape\":%d,\"extent\":[%u,%u],\"quality\":%d,"
                    "\"levels\":%d,\"radius_scale\":%g,\"luminance\":%g,\"threshold\":%g,\"gamma\":[%g,%g],\"remap\":%g,\"base\":[%u,%u],\"draws\":%zu,\"L\":%.9g%s}\n",
                    "ours","false",plan.refused.empty() && !plan.draws.empty()?"true":"false",(plan.refused.empty()?shape_refused:plan.refused).c_str(),skipped.c_str(),read?shape:-1,in.width,in.height,in.quality,in.levels,in.radius_scale,in.luminance,in.threshold,
                    in.composite_gamma,in.bloom_gamma,in.remap,plan.base_width,plan.base_height,plan.draws.size(),plan.luminance,CutsceneClockCell().c_str());Log(row);}}
            if(plan.refused.empty()){auto input=std::make_shared<NativeGlareInput>();input->plan=std::move(plan);input->programs=std::move(programs);input->film_grain=std::move(film_grain);cached=input;frame->yebis_glare=std::move(input);}}}}
    // W10 DF-4 (on by default; with the glare): the depth of field natively (W10-NATIVE-PASSES 9): the holder's creation from the
    // DoF quality (globals word 0x4C) and the render extent (cached), the lens from the scene camera (E69A0's record: fov, near,
    // far, aspect) and the globals block's DoF words (+0x1AC enable, +0x1B0 focus, +0x1B4 f-number, +0x1B8, +0x1BC, +0x1C0 the film
    // diagonal, +0x1D0/+0x1D4 the level limits, +0x240 the holder flags' index), the ten draws, the DoF table of the previous frame
    // (CustomDofLutCopy's, written at a frame's end for the next; the current one on the first frame). Refused with a row at each
    // change of the outcome: no depth-of-field branch, an unwitnessed configuration (yebis::BuildDof), the programmes.
    {static const bool dof_flag=NativeYebisPass(L"dof");
        if(dof_flag && frame->yebis_glare && scene->camera.width && scene->camera.height){std::array<uint8_t,0x370> block{};
            const auto& data=DofImageData();const bool read=scene->yebis && scene->yebis->globals_read;if(read)block=scene->yebis->globals;   // Fable review: the collect's snapshot
            const auto integer=[&](size_t at){int32_t i;std::memcpy(&i,block.data()+at,4);return i;};
            const auto& c=scene->camera;std::string refused;std::shared_ptr<NativeDofInput> input;
            const auto post=NativePostExtent(c);   // I-0111: the depth of field at the output extent
            static std::mutex dof_lock;static std::string setup_key;static ds3native::yebis::DofSetup setup;static std::vector<float> previous_table;
            const std::lock_guard hold_dof(dof_lock);
            if(!data.ok)refused="executable data";else if(!read)refused="globals";else if(c.projection_flags)refused="camera lens";
            else{
                const std::string key=std::to_string(post[0])+"x"+std::to_string(post[1])+"/"+std::to_string(integer(0x4C));
                if(key!=setup_key){setup=ds3native::yebis::BuildDofSetup(data.setup,post[0],post[1],integer(0x4C),7);setup_key=key;previous_table.clear();}
                if(!setup.refused.empty())refused="setup: "+setup.refused;}
            if(refused.empty()){
                const auto in=NativeDofLensInputs(c,block);   // I-0138 tranche 8d: shared with Alpha_R's depth spaces
                const auto lens=ds3native::yebis::DofLensState(data.k,in);
                const int flags_index=integer(0x240);static constexpr uint32_t holder_flags[5]{0,0,2,6,6};   // FUN_1416CBFE0 -> FUN_1416FDD80
                // I-0138 part 4b: an index of -1 (the cutscene records' edge quality) takes the holder's creation flags (+0xC8),
                // as FUN_1416cbfe0 does on YebisDofCreationFlagsTable_1416fdd80's 0x10000000; over 4 the original reads past its table.
                if(!lens.branch)refused="depth of field off";
                else if(flags_index<-1 || flags_index>4)refused="holder flags";
                else{
                    auto f=ds3native::yebis::NativeDofFrame(setup,lens,integer(0x1D0),integer(0x1D4),flags_index<0?setup.creation_flags:holder_flags[flags_index]);
                    auto plan=ds3native::yebis::BuildDof(data.draw,f);
                    if(!plan.refused.empty())refused=plan.refused;
                    else{ds3light::CustomDofInput t;
                        if(f.table_width!=2048 || !f.table_height)refused="DoF table extent";   // cbComputeCustomDof holds g_Coc[2048]
                        else if(!ds3light::CustomDofConstants(t))refused="DoF table constants";
                        else{t.width=f.table_width;t.height=f.table_height;t.near_plane=lens.near_plane;t.far_plane=lens.far_plane;
                            t.lens={lens.focus,lens.words[0],lens.words[1],1.0f/lens.words[0],-(lens.words[1]/lens.words[0])};t.exposure=lens.enable?lens.scale:0.0f;
                            for(uint32_t w=0;w<11;++w)std::memcpy(&t.dof[w],block.data()+0x2D4+4*w,4);
                            std::vector<uint8_t> cb(0x2010);ds3light::CustomDofTable(t,cb.data());
                            std::vector<float> table(size_t(f.table_width)*f.table_height);
                            for(uint32_t y=0;y<f.table_height;++y)std::memcpy(table.data()+size_t(y)*f.table_width,cb.data()+0x10,size_t(f.table_width)*4);
                            auto chosen=ChooseNativePostProgrammes(plan);   // I-0138 tranche 4: our own programmes
                            if(!chosen.programs)refused=std::move(chosen.refused);
                            else{input=std::make_shared<NativeDofInput>();input->plan=std::move(plan);input->programs=std::move(chosen.programs);
                                input->table=previous_table.size()==table.size()?previous_table:table;previous_table=std::move(table);}}}}}
            {static std::string last="unset";const std::string now=refused+"/"+std::to_string(integer(0x4C))+"/"+std::to_string(c.width)+"x"+std::to_string(c.height);
                if(now!=last){last=now;char row[400]{};sprintf_s(row,"{\"event\":\"native_yebis_dof\",\"programmes\":\"%s\",\"verify\":%s,\"on\":%s,\"refused\":\"%.120s\",\"quality\":%d,\"base\":[%u,%u],\"draws\":%zu}\n",
                    "ours","false",input?"true":"false",refused.c_str(),integer(0x4C),setup.base_width,setup.base_height,input?input->plan.pre.size()+input->plan.draws.size():size_t(0));Log(row);}}
            if(input)frame->yebis_dof=std::move(input);else previous_table.clear();}}   // review: after a refusal the next DoF frame starts on its own table
    if(!offscreen){auto mask=std::make_shared<NativeShadowMaskInput>();   // I-0126 R7c (D-0251): the main view's shadow mask inputs, the SSAO's removed
        // D-0180: the mask t32 (GXRend_CompositeShadowMask), which the original draws whichever the AO: the composite's two executable
        // pairs (I-0131 R10a: the raster sun layer's rotation image no longer taken).
        mask->ready=Read(image+0x3D7F690,mask->composite[0].data(),16) && Read(image+0x3D7FC00,mask->composite[0].data()+16,16) &&
            Read(image+0x3D7F370,mask->composite[1].data(),16) && Read(image+0x3D7FE70,mask->composite[1].data()+16,16);   // the final review's L4: no image gate
        frame->mask=std::move(mask);}
    if(scene->velocity && !offscreen){auto velocity=std::make_shared<NativeVelocityInput>();
        velocity->current_to_previous=scene->velocity->current_to_previous;velocity->scale=scene->velocity->scale;
        velocity->reset=scene->velocity->reset || scene->rt_release;   // I-0132 R11e: the arrival's released frame restarts the histories
        velocity->clip_to_previous_clip=scene->velocity->clip_to_previous_clip;
        // I-0136: the rows in their own depth and the occlusion pass (velo-depth-legacy.flag: the former test against the scene depth).
        const auto& camera=scene->camera;const double far_plane=camera.far_plane,near_plane=camera.near_plane;
        velocity->own_depth=!VeloDepthLegacy();   // the rows' state above follows the same flag
        velocity->occlusion={float(std::sqrt(double(camera.camera_pose[12])*camera.camera_pose[12]+double(camera.camera_pose[13])*camera.camera_pose[13]+
            double(camera.camera_pose[14])*camera.camera_pose[14])),camera.near_plane,near_plane>0 && far_plane>near_plane?float(far_plane/(far_plane-near_plane)):0.f,0.f};   // 0: no lens, no row removed
        frame->velocity=std::move(velocity);}
    else{frame->velocity_draws.clear();
        // I-0140 tranche 40 (D-0305): a menu view's TAA reads its camera motion alone (no object rows: the resolve's camera path on
        // every pixel, the scale unread there), the reset its first frame's or a cut's.
        if(offscreen && scene->menu_motion){auto velocity=std::make_shared<NativeVelocityInput>();
            velocity->current_to_previous=scene->menu_motion->current_to_previous;velocity->reset=scene->menu_motion->reset;velocity->scale={1,-1,0,1};
            velocity->clip_to_previous_clip=scene->menu_motion->clip_to_previous_clip;   // DLSS's constants on a DLAA view
            velocity->still_to_previous=scene->menu_motion->still_to_previous;   // a repeated presentation's (I-0140 tranche 40)
            velocity->own_depth=false;frame->velocity=std::move(velocity);}}
    // I-0140 tranche 40 (D-0305): a menu view's DLAA, the collect's choice, with its motion (the renderer's TAA if DLSS refuses it).
    frame->dlss=offscreen?scene->menu_motion && scene->menu_motion->dlss && frame->velocity:scene->dlss;   // I-0111 tranche 4 step 2 (final review M1: the collect's decision)
    frame->taa=scene->taa && (!offscreen || frame->velocity);frame->sharpen=NativeSharpen();   // I-0132 R11j; I-0140 tranche 40: a menu view's too (with its motion)
    if(offscreen && scene->menu_motion)frame->taa_kernel=scene->menu_motion->kernel;   // I-0140 tranche 41 (D-0306)
    else if(!offscreen){frame->taa_kernel=NativeTaaTest().kernel;frame->taa_reset_always=NativeTaaTest().reset_always;}   // I-0138 tranche 9: taa-kernel.txt (0 without it: the stage's 2.29), taa-reset-always.flag
    frame->taa_reactive=NativeTaaReactive();   // I-0113 tranche 39
    frame->taa_reactive_view=NativeTaaReactiveView();   // I-0113 tranche 41
    if(!offscreen){frame->rt_scene=scene->rt_scene;frame->rt_hold=scene->rt_hold;}   // I-0117 (R2 T1); I-0132 R11e: the arrival held
    else if(scene->rt_scene && scene->shadow_frame)frame->rt_scene=scene->rt_scene;   // I-0140 t31: a Player menu view's traced self-shadow (its own scene)
    // I-0125 R6c (R6-PLAN §6): the water's plane when this scene's collect chose the traced water, with the
    // reflection target (made by that collect or a later one: the output's half extent either way).
    if(!offscreen && scene->rt_scene && scene->rt_water_plane){const std::lock_guard hold(native_reflection.lock);
        if(native_reflection.target)frame->rt_water=NativeModelFrameInput::RtWater{native_reflection.target,*scene->rt_water_plane};}
    // W10 MB-4 (on by default; drawn with the glare plan's Alpha_R and the velocity input): the motion blur natively (W10-NATIVE-PASSES 10). E69A0's
    // setters from the globals block's MotionBlur words (+0x1DC Enable, +0x1E0 mode, +0x1E4 BlurTimeRatio, +0x1E8 MaxBlurLength, +0x1EC
    // NumBaseSamples, +0x1F0 MaxRecurrences after the option cap, +0x1F4 SampleInterleaved, +0x200 the camera-threshold flag, +0x204
    // Bidirectional, +0x2AC/+0x2B0 the falloff distances, +0x350 MotionBlurScale x factor), the camera (pose, the previous pose by the
    // velocity's cut rule, fov, near, far, aspect) and the library's own words observed at m37, m31, m50 (S+0x340 2, S+0xA00 0.15,
    // S+0xA04 -0.01, S+0xA08 0: INFERRED constants, Reference1676-1678); the scene alpha's depth decoded in Alpha_R's spaces (tranche
    // 8d below). The internal velocity's extent: Initialize's (int)(W x 0.5 + 0.99) and (int)(width / aspect + 0.499999) (INFERRED the
    // 0.5 default of S+0x3E8 0). Refused with a row at each change of the outcome: the flag off, another mode, the threshold flag, the
    // falloff, the globals, another lens, no glare plan (I-0138 tranche 8c: the row is written without one), no Alpha_R (a guard; also the
    // case without the executable's DoF words, whose own « executable data » below is then a guard), an unserved plan, the programmes. I-0138 tranche 8d: no longer refused without the Dof record's Enable (the
    // library's blur never reads it, READ); the scene alpha's depth is decoded in Alpha_R's spaces (NativeAlphaRInputs: the focus in the
    // depth-of-field branch, the near plane outside it), the row's « depth » naming which.
    {static const bool motionblur_flag=NativeYebisPass(L"motionblur");
        if(motionblur_flag && !offscreen && frame->velocity && scene->velocity && scene->camera.width && scene->camera.height){
            namespace y=ds3native::yebis;std::array<uint8_t,0x370> block{};const bool read=scene->yebis && scene->yebis->globals_read;if(read)block=scene->yebis->globals;   // Fable review: the collect's snapshot
            const auto word=[&](size_t at){float f;std::memcpy(&f,block.data()+at,4);return f;};const auto integer=[&](size_t at){int32_t i;std::memcpy(&i,block.data()+at,4);return i;};
            const auto& c=scene->camera;std::string refused;std::shared_ptr<NativeMotionBlurInput> input;y::MotionBlurPlan plan;const char* depth_space="";
            const float falloff_min=word(0x2AC),falloff_max=word(0x2B0);   // S+0x9F0, S+0x9F4: FUN_1416EAE70's test
            const bool falloff=0.0f<falloff_max && 0.0f<falloff_min && falloff_max!=falloff_min && (falloff_min<=falloff_max || c.near_plane<falloff_min) &&
                (falloff_max<=falloff_min || falloff_min<c.far_plane);
            if(!read)refused="globals";else if(c.projection_flags)refused="camera lens";else if(!integer(0x1DC))refused="motion blur off";
            else if(integer(0x1E0)!=2)refused="mode";else if(integer(0x200))refused="camera threshold flag";else if(falloff)refused="falloff";
            else if(!frame->yebis_glare)refused="no glare plan";   // I-0138 tranche 8c: no glare plan this frame: refused (the glare row names why; not met since 2026-11-16), or the glare block not run (its flag off, no tone-map input)
            else if(frame->yebis_glare->plan.pre.empty())refused="no Alpha_R";   // I-0138 tranche 8c: a guard (the glare block draws it for the blur)
            else{
                const auto post=NativePostExtent(c);   // I-0111: the blur and its half-size object velocity at the output extent
                y::MotionBlurInputs in;in.width=post[0];in.height=post[1];
                const float aspect=c.explicit_aspect?c.explicit_aspect:float(c.width)/float(c.height);
                in.velocity_width=uint32_t(int64_t(float(post[0])*0.5f+0.99f));in.velocity_height=uint32_t(int64_t(float(in.velocity_width)/aspect+0.499999f));
                auto& cam=in.camera;cam.near_plane=c.near_plane;cam.far_plane=c.far_plane;cam.tan_half=std::tan(c.vertical_fov*0.5f);cam.aspect=aspect;
                cam.previous_tan_half=cam.tan_half;   // the previous camera record's field of view: INFERRED equal (no zoom witnessed)
                std::memcpy(cam.current.data(),c.camera_pose.data(),64);std::memcpy(cam.previous.data(),scene->velocity->previous_pose.data(),64);
                in.time_ratio=std::max(0.0f,word(0x1E4));in.max_length=std::max(0.0f,word(0x1E8));in.base_samples=std::max(1,integer(0x1EC));
                in.max_recurrences=integer(0x1F0);in.bidirectional=block[0x204]!=0;in.interleave=std::max(1.0f,word(0x1F4));
                in.capability=2;in.similarity=0.15f;in.depth_threshold=-0.01f;in.depth_threshold_far=0.0f;
                float s=word(0x350)*word(0x1E8);if(s<=1.0e-4f)s=1.0e-4f;in.velocity_scale={s,s};   // A7a: s = P+0x350 x P+0x1E8
                y::AlphaRInputs alpha;bool branch{};
                if(!NativeAlphaRInputs(c,block,alpha,&branch))refused="executable data";
                const auto& spaces=alpha.spaces;depth_space=refused.empty()?branch?"focus":"near":"";
                in.object_decode=y::DepthConversion(spaces,6,0,1.0f,0.0f);in.scene_decode=y::DepthConversion(spaces,2,0,1.0f,0.0f);
                if(refused.empty()){plan=y::BuildMotionBlur(y::MotionBlurData{},in);refused=plan.refused;}
                if(refused.empty()){auto chosen=ChooseNativePostProgrammes(plan);   // I-0138 tranche 4: our own programmes
                    if(!chosen.programs)refused=std::move(chosen.refused);
                    else{input=std::make_shared<NativeMotionBlurInput>();input->plan=plan;input->programs=std::move(chosen.programs);
                        input->object_width=post[0]>>1;input->object_height=post[1]>>1;}}}
            {static std::string last="unset";const std::string now=refused+"/"+depth_space+"/"+std::to_string(plan.passes)+"/"+std::to_string(plan.taps)+"/"+NativeWordsList(reinterpret_cast<const uint32_t*>(block.data()+0x1DC),11)+
                    NativeWordsList(reinterpret_cast<const uint32_t*>(block.data()+0x350),1)+std::to_string(c.width)+"x"+std::to_string(c.height);
                if(now!=last){last=now;char row[520]{};sprintf_s(row,"{\"event\":\"native_yebis_motionblur\",\"programmes\":\"%s\",\"verify\":%s,\"on\":%s,\"refused\":\"%.120s\",\"depth\":\"%s\",\"enable\":%d,\"mode\":%d,\"ratio\":%g,"
                    "\"length\":%g,\"samples\":%d,\"recurrences\":%d,\"scale\":%g,\"passes\":%d,\"taps\":%d,\"step\":%.9g,\"draws\":%zu}\n","ours","false",input?"true":"false",refused.c_str(),depth_space,
                    integer(0x1DC),integer(0x1E0),word(0x1E4),word(0x1E8),integer(0x1EC),integer(0x1F0),word(0x350),plan.passes,plan.taps,plan.step,plan.draws.size());Log(row);}}
            if(input)frame->yebis_motionblur=std::move(input);}}
    // W5a SW-6a; I-0131 R10a: the sun's cascades and the local lights' atlas are removed (no casters, atlases, cascade views or blur
    // taps). What a frame keeps of the shadow state is what the traced sun and the shadowed sun pass read: the shadow blocks
    // (the camera words and the shadow scene, whose +0x1D0 FC_ShadowColor the sun pass reads) and the sun's direction. The gate is
    // the original's (the main view, the shadow frame on, the camera's projection invertible; I-0132 R11e: no-sun-shadow.flag removed);
    // a frame without it has no traced sun: judged unserved (cause 1).
    // I-0140 tranche 31 (D-0303): a Player menu view with its traced scene takes the same blocks and sun (its own ShadowParam's:
    // CollectNativeMenuSelfShadow), the menu views' camera projection invertible alike.
    {   if((!offscreen || frame->rt_scene) && scene->shadow_frame && scene->shadow_frame->on){
            {const auto& sf=*scene->shadow_frame;const auto& cc=scene->camera_constants;
                std::array<float,16> projection{};for(size_t r=0;r<4;++r)for(size_t c=0;c<4;++c)std::memcpy(&projection[4*r+c],cc.bytes.data()+256+4*(4*c+r),4);
                VelocityMatrix dp{},inverse{};for(int i=0;i<16;++i)dp[i]=projection[i];
                if(VelocityInverse(dp,inverse)){
                    std::memcpy(frame->shadow_scene.data(),cc.bytes.data(),960);std::memcpy(frame->shadow_scene.data()+0x1C0,sf.scene_words.data(),sf.scene_words.size());
                    frame->rt_sun=sf.sun;frame->shadow_blocks=true;   // I-0117 R5: the traced sun shadow's direction; R5a: its gate
                    if(offscreen){frame->self_shadow_blur_passes=sf.blur_passes;frame->self_shadow_blur_scale=sf.blur_scale;}}}}}   // I-0140 tranche 35
    {static LONG64 velocity_samples{};if(frame->velocity && !offscreen && [&]{const auto n=InterlockedIncrement64(&velocity_samples);return n<=6 || n%300==0;}()){
        size_t rows{};for(const auto& prepared:scene->prepared_meshes)rows+=prepared.engine_pass==0x34;
        size_t entities{};for(const auto& model:scene->models)entities+=model.velocity_rows;
        char row[224]{};sprintf_s(row,"{\"event\":\"native_velocity_draws\",\"models_with_velocity\":%zu,\"prepared_rows\":%zu,\"draws\":%zu,\"reset\":%s}\n",
            entities,rows,frame->velocity_draws.size(),frame->velocity->reset?"true":"false");Log(row);}}
    if(!frame->paints.empty()){static LONG64 rows{};if(const auto n=InterlockedIncrement64(&rows);n<=24 || n%100==0){char row[384]{};   // D2
        size_t draws{},depth_draws{},erase_jobs{};for(const auto& job:frame->paints){draws+=job.draws.size();depth_draws+=job.depth_draws.size();erase_jobs+=job.erase;}
        sprintf_s(row,"{\"event\":\"native_paint_frame\",\"n\":%lld,\"jobs\":%zu,\"rows\":%llu,\"draws\":%zu,\"no_set\":%llu,\"refused\":%llu,\"no_t41\":%llu,\"depth_draws\":%zu,\"depth_refused\":%llu,\"erase_jobs\":%zu,\"erase_rows\":%llu,\"erase_refused\":%llu,\"no_erase_programme\":%llu,\"erase_no_t41\":%llu}\n",
            n,frame->paints.size(),static_cast<unsigned long long>(paint_counts[0]),draws,static_cast<unsigned long long>(paint_counts[2]),
            static_cast<unsigned long long>(paint_counts[3]),static_cast<unsigned long long>(paint_counts[4]),depth_draws,static_cast<unsigned long long>(paint_counts[5]),
            erase_jobs,static_cast<unsigned long long>(paint_counts[6]),static_cast<unsigned long long>(paint_counts[7]),static_cast<unsigned long long>(paint_counts[8]),static_cast<unsigned long long>(paint_counts[9]));Log(row);}}
    // Step review: a frame dropped (nullptr: no G-buffer draw and not a lit main view of forward draws, I-0141 tranche 20) hands its
    // fresh paint events back to the scene's next presentation; a kept forward-only frame is counted with native_forward_only_frames.
    if(frame->draws.empty() && (offscreen || !frame->lighting || frame->forward_draws.empty())){if(paints_fresh)scene->paints_taken->store(false);return nullptr;}
    if(frame->draws.empty()){static LONG64 forward_only_rows{};if(const auto n=InterlockedIncrement64(&forward_only_rows);n<=4 || n%600==0){char row[200]{};
        sprintf_s(row,"{\"event\":\"native_forward_only_frames\",\"n\":%lld,\"forward\":%zu,\"water\":%zu,\"effects\":%zu}\n",static_cast<long long>(n),
            frame->forward_draws.size(),frame->water_draws.size(),frame->effects?frame->effects->draws.size():size_t(0));Log(row);}}
    return frame;
}
}  // namespace
