// The model frame's inputs: vertex elements, register blocks, the kept draws, decal and paint draws, the pop-in frame rows.
// Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part (12_scene_collect.inc
// before it).
#pragma once
#include "12_scene_collect.inc"
namespace {
uint32_t NativeVertexElementBytes(DXGI_FORMAT format) noexcept {
    // SDK uncompressed byte widths; actual IA format support is checked by
    // the existing draw component on the supplied startup device.
    const auto value=static_cast<uint32_t>(format);
    if(value>=1 && value<=4)return 16;
    if(value>=5 && value<=8)return 12;
    if(value>=9 && value<=22)return 8;
    if(value>=23 && value<=47)return 4;
    if(value>=48 && value<=59)return 2;
    if(value>=60 && value<=65)return 1;
    if(value>=67 && value<=69)return 4;
    if(value==85 || value==86)return 2;
    if(value>=87 && value<=93)return 4;
    return 0;
}
// 1034 witness: which shader texture a selected row lacks and why (1 bindings not
// ready, 2 register not bound, 3 no texture, 4 texture not resident).
template<class Mesh> void TextureWitness(const Mesh& mesh,const std::string& name,uint32_t slot,size_t stage,unsigned reason,unsigned status,bool offscreen) {   // cF.1: the view kind as a parameter, not the thread-local
    static LONG64 samples{},offscreen_samples{};
    if(InterlockedIncrement64(offscreen?&offscreen_samples:&samples)>16)return;
    std::string family;for(const auto ch:mesh.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
    char row[512]{};sprintf_s(row,"{\"event\":\"native_selected_texture_unavailable\",\"offscreen\":%s,\"stage\":%zu,\"slot\":%u,\"name\":\"%.80s\",\"reason\":%u,\"status\":%u,\"family\":\"%.120s\"}\n",
        offscreen?"true":"false",stage,slot,name.c_str(),reason,status,family.c_str());Log(row);
}
// I-0105 (Lightning Spear's straight core missing): an FFX model particle's row refused for a texture, once per data name and
// texture name (up to 64): the reason as TextureWitness's (1 bindings not ready, 2 register not bound, 3 none, 4 not resident).
void EffectModelTextureWitness(uintptr_t model,const std::string& texture,unsigned status,unsigned reason) {
    static std::mutex seen_lock;static std::set<std::string> seen;static std::atomic<bool> seen_full{};std::string name;
    static std::atomic<uint64_t> calls{};   // reviews: a refused row repeats every frame; the first 4096 calls read a name, then one in 600
    if(seen_full.load(std::memory_order_relaxed))return;   // no name read once the 64 rows are written
    if(const auto n=calls.fetch_add(1,std::memory_order_relaxed);n>=4096 && n%600)return;
    for(const auto ch:NativeModelDataName(model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
    {const std::lock_guard hold(seen_lock);if(seen.size()>=64){seen_full=true;return;}if(!seen.insert(name+"|"+texture).second)return;}
    char row[300]{};sprintf_s(row,"{\"event\":\"native_effect_model_texture_missing\",\"model\":\"%.60s\",\"texture\":\"%.60s\",\"reason\":%u,\"status\":%u}\n",
        name.c_str(),texture.c_str(),reason,status);Log(row);}
// 1068 witness (characters): per SprjAsmModelDrawEntity part, admission refusal
// (1 zero fade, 2 bindings, 3 instance streams, 4 geometry, 5 no meshes), pass
// gates (PrepareNativeSelectedPasses refusal index) and draw-stage outcomes
// (prepared/drawn, failures: constants, textures, palettes, layouts, samplers, interfaces).
struct NativeAsmOutcome { uint32_t prepared{},drawn{};std::array<uint32_t,6> fail{}; };
void LogNativeAsmSummary(const NativeSelectedScene& scene,const std::map<int32_t,NativeAsmOutcome>& outcomes,
    const std::map<std::string,NativeAsmOutcome>& family_outcomes={}) {
    if(scene.asm_rejected.empty() && scene.asm_gates.empty() && outcomes.empty())return;
    // PrepareNativeModelFrame runs per selected scene: sample every 30th call, at most 40 rows.
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);   // I-0120 B2: the row count is native_asm_summary_logged (10), read by the feeders
    if(!(n<=3 || n%30==0) || native_asm_summary_logged.fetch_add(1,std::memory_order_relaxed)+1>40)return;
    std::map<int32_t,std::map<int,uint32_t>> gates;for(const auto& [part,code]:scene.asm_gates)++gates[part][code];
    std::string row="{\"event\":\"native_asm_part_outcomes\",\"serial\":"+std::to_string(scene.serial)+",\"rejected\":[";
    for(size_t i=0;i<scene.asm_rejected.size() && i<64;++i)row+=(i?",":"")+std::string("[")+std::to_string(scene.asm_rejected[i].first)+","+std::to_string(scene.asm_rejected[i].second)+"]";
    row+="],\"gates\":{";bool first=true;
    for(const auto& [part,codes]:gates){row+=(first?"":",")+std::string("\"")+std::to_string(part)+"\":{";first=false;bool inner=true;
        for(const auto& [code,count]:codes){row+=(inner?"":",")+std::string("\"")+std::to_string(code)+"\":"+std::to_string(count);inner=false;}row+="}";}
    row+="},\"draws\":{";first=true;
    for(const auto& [part,o]:outcomes){row+=(first?"":",")+std::string("\"")+std::to_string(part)+"\":["+std::to_string(o.prepared)+","+std::to_string(o.drawn);first=false;
        for(const auto value:o.fail)row+=","+std::to_string(value);row+="]";}
    // 1118: the same outcome by material family: pass gates, then [prepared, drawn, failures].
    row+="},\"families\":{";first=true;std::set<std::string> keys;
    for(const auto& [key,codes]:scene.asm_family_gates)keys.insert(key);for(const auto& [key,o]:family_outcomes)keys.insert(key);
    for(const auto& key:keys){row+=(first?"":",")+std::string("\"")+key+"\":{\"gates\":{";first=false;bool inner=true;
        const auto found=scene.asm_family_gates.find(key);
        if(found!=scene.asm_family_gates.end())for(const auto& [code,count]:found->second){row+=(inner?"":",")+std::string("\"")+std::to_string(code)+"\":"+std::to_string(count);inner=false;}
        row+="},\"draw\":[";const auto drawn=family_outcomes.find(key);
        if(drawn!=family_outcomes.end()){row+=std::to_string(drawn->second.prepared)+","+std::to_string(drawn->second.drawn);for(const auto value:drawn->second.fail)row+=","+std::to_string(value);}
        row+="]}";}
    row+="}}\n";Log(row.c_str());
}
// PREPARE-REDUCTION cP1: the recipe's draw programme set, made once per Programs (the vertex elements are its layout's).
std::shared_ptr<const NativeModelProgramSet> NativeProgramSetOf(const NativeModelLayout::Programs& programs,size_t recipe,
    const std::vector<ds3content::ModelVertexElement>& elements) {
    const std::lock_guard hold(programs.program_set_lock);auto& set=programs.program_sets.at(recipe);
    if(!set){ds3content::CpuShaderProgram vertex,pixel;vertex.stage=ds3content::ShaderStage::Vertex;vertex.model_major=5;
        vertex.bytecode=programs.ProgramBytes(recipe,0);pixel.stage=ds3content::ShaderStage::Pixel;pixel.model_major=5;
        pixel.bytecode=programs.ProgramBytes(recipe,1);
        // W7 TS-2b: a tessellated recipe's set carries its hull and domain programmes (the ModelTess family).
        ds3content::CpuShaderProgram hull,domain;
        if(NativeTessellatedRecipe(recipe)){hull.stage=ds3content::ShaderStage::Hull;hull.model_major=5;hull.bytecode=programs.ProgramBytes(recipe,2);
            domain.stage=ds3content::ShaderStage::Domain;domain.model_major=5;domain.bytecode=programs.ProgramBytes(recipe,3);}
        set=MakeNativeModelProgramSet(std::move(vertex),std::move(pixel),elements,std::move(hull),std::move(domain));}
    return set;
}
// I-0134: the recipe's cloth Velo set (the native vertex programme reading the previous simulated copy, the game's pixel
// programme), made once per Programs from the recipe's own set; null when its vertex programme is not a cloth Velo one.
std::shared_ptr<const NativeModelProgramSet> NativeClothVelocityProgramSetOf(const NativeModelLayout::Programs& programs,size_t recipe,
    const NativeModelProgramSet& base) {
    const std::lock_guard hold(programs.program_set_lock);
    if(!programs.cloth_velocity_tried.at(recipe)){programs.cloth_velocity_tried[recipe]=true;
        if(const auto& shader=programs.interfaces[recipe][0])programs.cloth_velocity_sets[recipe]=MakeNativeClothVelocitySet(base,*shader);
        static LONG64 sets{};if(InterlockedIncrement64(&sets)<=24){char row[256]{};
            sprintf_s(row,"{\"event\":\"native_cloth_velocity_set\",\"family\":\"%.160s\",\"recipe\":%zu,\"native\":%s}\n",NativeFamilyKey(programs).c_str(),recipe,
                programs.cloth_velocity_sets[recipe]?"true":"false");Log(row);}}
    return programs.cloth_velocity_sets[recipe];
}
// Workstream K R8a: one register of a draw from the programme's plan for (recipe, stage, slot, name), made once; the
// same block as PrepareReadShaderRegister by construction.
ds3content::ShaderRegisterBlock PrepareNativeProgramRegister(const NativeModelLayout::Programs& programs,size_t recipe,size_t stage,
    uint32_t slot,std::span<const ds3content::ShaderConstantRange> ranges,std::string_view name={}) {
    const auto& shader=*programs.interfaces[recipe][stage];const auto& reads=programs.ReadsOf(recipe,stage);
    if(slot>=14)return ds3content::PrepareReadShaderRegister(shader,reads,slot,ranges,name);
    std::shared_ptr<const ds3content::ShaderRegisterPlan> plan;
    {const std::lock_guard hold(programs.draw_plan_lock);auto& plans=programs.draw_plans[recipe][stage][slot];
        for(const auto& entry:plans)if(entry.name==name){plan=entry.plan;break;}
        if(!plan){plan=std::make_shared<const ds3content::ShaderRegisterPlan>(ds3content::PlanReadShaderRegister(shader,reads,slot,name));
            plans.push_back({std::string(name),plan});}}
    return ds3content::PackShaderRegisterPlan(shader,*plan,ranges);
}
// C7-1: the scene (b8), view (b0) and instance (b4) blocks of a mesh, from the scene's and the model's tables by programme
// key; a null block is the default one, as before.
const SharedRegisterBlock& NativeMeshSceneBlock(const NativeSelectedScene& scene,const NativeSelectedMesh& mesh,size_t recipe,size_t stage) {
    static const SharedRegisterBlock none;
    if(!mesh.programs || !mesh.programs->interfaces[recipe][stage])return none;
    const auto found=scene.scene_block_table.find(reinterpret_cast<const void*>(NativeProgramKey(*mesh.programs,recipe,stage)));
    return found?found->block:none;
}
const SharedRegisterBlock& NativeMeshViewBlock(const NativeSelectedScene& scene,const NativeSelectedMesh& mesh,size_t recipe) {
    static const SharedRegisterBlock none;
    if(!mesh.programs || !mesh.programs->interfaces[recipe][0])return none;
    const auto found=scene.view_block_table.find(reinterpret_cast<const void*>(NativeProgramKey(*mesh.programs,recipe,0)));
    return found?found->block:none;
}
// C4-2: the key of a main-view row (entity, model, mesh, fade, engine pass, recipe, occurrence) and its hash (the kept draws).
namespace {
struct NativeDryRowKey {uint64_t entity{},model{};size_t mesh{};uint32_t fade{},pass{},recipe{},occurrence{};bool operator==(const NativeDryRowKey&) const=default;};
struct NativeDryRowHash {size_t operator()(const NativeDryRowKey& k) const noexcept {
    uint64_t h=k.entity*0x9E3779B97F4A7C15ull^k.model;h=h*31+k.mesh;h=h*31+k.fade;h=h*31+k.pass;h=h*31+k.recipe;h=h*31+k.occurrence;return size_t(h^(h>>29));}};
}
// C4-2 (PERSISTENT-RECORDS-DESIGN 2): the main view's prepared draws kept across prepares. A row (entity, model, mesh, fade,
// engine pass, recipe, occurrence) takes its kept draw when everything the row reads is unchanged: its mesh by identity
// (the C7-2 mesh set is immutable and held by the model inputs below), the model's inputs by value (a version number per
// model, bumped when they differ), the row's tiling values and instance blocks, and the scene inputs every row reads (the
// epoch below; a change drops every draw). The frame registers are produced again for each hit, so the view's canonical
// blocks are this frame's. A kept draw holds what it references (mesh set, geometry, streams, sampler bank), not the scene.
// Main view only, one prepare at a time (under the lock); one frame slot only (C4-2a: a kept draw's bounded CBVs are
// rewritten when the previous frame is complete). Failures are never kept.
namespace {
struct NativeKeptModelInputs {
    std::shared_ptr<const void> meshes,geometry,pose,previous_pose,streams;
    std::shared_ptr<const NativeSelectedModel::EngineBytes> engine;
    std::shared_ptr<const NativeSelectedModel::ForwardLight> forward;
    std::shared_ptr<const NativeRegionIbl> region;   // I-0113 tranche 14: t30/t31 are bound when a draw is built (the memo keeps the pointer while nothing changes)
    std::array<float,12> placement{};std::optional<std::array<float,24>> cloth_world;
    std::optional<uint32_t> ordinary_category;std::optional<std::array<uint8_t,16>> draw_colour;std::optional<std::array<uint8_t,64>> texture_scroll;
    std::array<ingress::ModelFadeIntent,2> fades{};
    bool animation{},phantom{};uint8_t phantom_kind{};float phantom_fade{};int32_t asm_part{};
};
template<class T> bool NativeSameBits(const T& a,const T& b) noexcept {return !std::memcmp(&a,&b,sizeof(T));}
template<class T> bool NativeSameBits(const std::optional<T>& a,const std::optional<T>& b) noexcept {
    return a.has_value()==b.has_value() && (!a || NativeSameBits(*a,*b));}
bool NativeSameFade(const ingress::ModelFadeIntent& a,const ingress::ModelFadeIntent& b) noexcept {
    return a.variant==b.variant && a.branch==b.branch && NativeSameBits(a.weight,b.weight) && NativeSameBits(a.opacity,b.opacity) &&
        a.fade_enabled==b.fade_enabled && a.alternate_fade_enabled==b.alternate_fade_enabled;}
// The first model input that differs (an index into NativeKeptModelFields for the outcome row), or -1 when all are equal.
constexpr const char* NativeKeptModelFields[]{"meshes","geometry","pose","previous_pose","streams","engine_group","engine_animation","engine_flags",
    "forward","placement","cloth_world","category","draw_colour","scroll","fades","animation","phantom","asm_part","region_ibl"};
int NativeKeptModelDifference(const NativeKeptModelInputs& a,const NativeKeptModelInputs& b) noexcept {
    if(a.meshes!=b.meshes)return 0;if(a.geometry!=b.geometry)return 1;if(a.pose!=b.pose)return 2;if(a.previous_pose!=b.previous_pose)return 3;
    if(a.streams!=b.streams)return 4;
    if(a.engine!=b.engine){if(!a.engine || !b.engine)return 7;const auto &x=*a.engine,&y=*b.engine;
        if(x.group!=y.group)return 5;if(x.animation_block!=y.animation_block)return 6;
        if(x.group_read!=y.group_read || x.block_flag_read!=y.block_flag_read || x.matrix_flag_read!=y.matrix_flag_read || x.block_read!=y.block_read ||
           x.block_flag!=y.block_flag || x.matrix_flag!=y.matrix_flag)return 7;}
    if(a.forward!=b.forward && (!a.forward || !b.forward || a.forward->light!=b.forward->light || a.forward->fog!=b.forward->fog))return 8;
    if(!NativeSameBits(a.placement,b.placement))return 9;if(!NativeSameBits(a.cloth_world,b.cloth_world))return 10;
    if(a.ordinary_category!=b.ordinary_category)return 11;if(a.draw_colour!=b.draw_colour)return 12;if(a.texture_scroll!=b.texture_scroll)return 13;
    if(!NativeSameFade(a.fades[0],b.fades[0]) || !NativeSameFade(a.fades[1],b.fades[1]))return 14;if(a.animation!=b.animation)return 15;
    if(a.phantom!=b.phantom || a.phantom_kind!=b.phantom_kind || !NativeSameBits(a.phantom_fade,b.phantom_fade))return 16;
    if(a.asm_part!=b.asm_part)return 17;
    if(a.region!=b.region)return 18;
    return -1;
}
// The scene inputs a row reads outside its model and mesh (the frame registers excepted: they are produced per hit).
struct NativeKeptEpoch {
    std::shared_ptr<const void> sampler_bank;uint64_t sampler_generation{};const void* device{};
    ds3content::NativeModelPixelPolicy pixel_policy{};std::optional<float> category_scalar;std::optional<std::array<uint8_t,16>> default_row;
    bool velocity{},simulation_time{},snow{};   // I-0137's step review M2: the snow inputs' presence, as the velocity's (their values per row)
    bool fade_mask{};   // I-0140 (gap 23, its second step review's M1): whether fading rows make the TAA's fade-mask pipeline (13b)
    bool operator==(const NativeKeptEpoch& o) const noexcept {
        return sampler_bank==o.sampler_bank && sampler_generation==o.sampler_generation && device==o.device && pixel_policy==o.pixel_policy &&
            NativeSameBits(category_scalar,o.category_scalar) && default_row==o.default_row && velocity==o.velocity && simulation_time==o.simulation_time &&
            snow==o.snow && fade_mask==o.fade_mask;}
};
// I-0105 review H1: cbModelParam's FC_SnowRate (ProduceNativeSnowRate) reads the scene snow rate, the Snow Model bank's built-in
// rate and the gparam layers: a kept draw that wrote it is rebuilt when one of them changes (every frame of a snow-rate transition).
// I-0120 B5: this stamp and the Velo rows' velocity scale are the draw's own, no longer the epoch's, so a blend of either rebuilds
// only the rows that wrote it (a gparam blend across a region boundary had rebuilt every row each frame of the blend).
struct NativeKeptSnow {
    std::array<float,2> rates{};std::shared_ptr<const void> banks;uint64_t generation{};   // held: a reloaded bank at the same address is seen
    bool operator==(const NativeKeptSnow& o) const noexcept {return NativeSameBits(rates,o.rates) && banks==o.banks && generation==o.generation;}
};
struct NativeKeptModel {NativeKeptModelInputs inputs;uint64_t version{},last_seen{};};
struct NativeKeptDraw {
    std::shared_ptr<const OwnedNativeModelDraw> draw;
    uint64_t model_version{},last_used{};
    ds3content::ModelTextureTiling tiling;
    std::array<SharedRegisterBlock,2> instance_blocks;   // the row's VS and PS instance blocks (b4) when it was built
    std::optional<std::array<float,4>> velocity_scale;   // B5: a Velo row's FC_DifColMul when it was built
    std::optional<NativeKeptSnow> snow;                  // B5: the snow inputs of a row whose cbModelParam holds FC_SnowRate
};
struct NativeModelKeyHash {size_t operator()(const std::pair<uint64_t,uint64_t>& k) const noexcept {
    const uint64_t h=k.first*0x9E3779B97F4A7C15ull^k.second;return size_t(h^(h>>29));}};
struct NativeKeptDraws {
    std::mutex lock;
    std::unordered_map<NativeDryRowKey,NativeKeptDraw,NativeDryRowHash> draws;
    std::unordered_map<std::pair<uint64_t,uint64_t>,NativeKeptModel,NativeModelKeyHash> models;
    std::optional<NativeKeptEpoch> epoch;
    uint64_t prepares{},versions{};std::atomic<uint64_t> resets_seen{};   // resets_seen: read before the lock by the early check
    std::array<uint64_t,9> outcomes{};   // since the last row: hits, new rows, model changed, row changed, epoch drops, two former shadow slots (zero since I-0102), frame-block refusals, rebuilt rows that took their kept draw's state (I-0090)
    std::array<uint64_t,std::size(NativeKeptModelFields)> model_changes{};   // models whose version moved, by the first differing input
};
// Never destroyed: a kept draw holds device objects, released by CloseNativeKeptDraws before the device closes.
NativeKeptDraws& native_kept_draws=*new NativeKeptDraws;
std::atomic<uint64_t> native_kept_draw_resets{};
bool native_kept_draws_disabled{};   // no-persistent-draws.flag (runner -NoPersistentDraws)
// I-0090: a kept row rebuilt because its model changed takes its kept draw's prepared state when the state inputs are the same
// (PrepareNativeModelDraw with a base); no-state-reuse.flag (A/B) prepares every rebuilt row in full.
bool native_state_reuse_disabled{};
// I-0070 (see the sweep after the rows): unused draws age out after 600 prepares, the least recently used beyond the bound go at
// the end of each prepare; within a prepare at most the limit (about 23 global descriptors a draw: 377k of the heap's 524k).
constexpr uint64_t native_kept_draw_unused=600;constexpr size_t native_kept_draw_bound=12288,native_kept_draw_limit=16384;
bool NativeSameRegisterBlock(const SharedRegisterBlock& a,const SharedRegisterBlock& b) noexcept {
    return a==b || (a && b && a->status==b->status && a->required==b->required && a->bytes==b->bytes);}
}
void NoteNativeKeptDrawReset() noexcept {native_kept_draw_resets.fetch_add(1,std::memory_order_acq_rel);}
uint64_t NativeKeptDrawResets() noexcept {return native_kept_draw_resets.load(std::memory_order_relaxed);}
// I-0096: a reset noted while no main view is prepared (the title after Quit Game, a loading screen) is applied here, from the
// frame's owner thread when no world was drawn; otherwise the last zone's kept draws held its textures and buffers (172 MB of
// m31's, i96-03) and pinned its heaps until the next zone's first prepare. Returns the draws released. Step review 1: the
// frame finished here may precede one the render worker is preparing (a zone's first, every row new): a busy lock is left
// to the next window, and a reset that prepare already applied is not applied again over its new draws.
size_t ApplyPendingNativeKeptDrawReset() {
    if(native_kept_draw_resets.load(std::memory_order_acquire)==native_kept_draws.resets_seen.load(std::memory_order_acquire))return 0;
    decltype(native_kept_draws.draws) draws;decltype(native_kept_draws.models) models;   // destroyed after the lock
    {const std::unique_lock hold(native_kept_draws.lock,std::try_to_lock);if(!hold.owns_lock())return 0;
        const auto resets=native_kept_draw_resets.load(std::memory_order_acquire);if(resets==native_kept_draws.resets_seen.load(std::memory_order_acquire))return 0;
        draws.swap(native_kept_draws.draws);models.swap(native_kept_draws.models);native_kept_draws.epoch.reset();native_kept_draws.resets_seen=resets;}
    return draws.size();
}
// Close (before the device closes): the kept draws are released; a prepare in progress finishes first.
void CloseNativeKeptDraws() {
    const std::lock_guard hold(native_kept_draws.lock);native_kept_draws.draws.clear();native_kept_draws.models.clear();native_kept_draws.epoch.reset();
}
// DECALS-WAVE1 D1: the deferred box decals of the main view (198910 per decal of the 51AA0 list), drawn after the G-buffer
// into the seven targets with the original DXBC pair. Constants: VS b10 and PS b10 (ds3content::ComputeDecalConstants, the
// transcription of 198910), PS b7 (1A86C0 over the 1A8580 state from the decal's material) and PS b11 (59470's blended SH rows).
struct NativeDecalChoice {uint32_t pixel{},vertex{};bool inside{};std::array<uint8_t,80> vs_b10{};std::array<uint8_t,416> ps_b10{};};
// 198910's view inputs from the native camera the G-buffer drew with: the world-to-view (the pose's inverse), the projection
// the camera block holds (FC_MatrixProj, stored for column vectors: transposed back), the pose as camera world, the lens and
// the render size. No emissive curve object is owned natively (the decals observed have none).
bool ProduceNativeDecalChoice(const NativeSelectedScene& scene,const NativeSelectedScene::DecalSnapshot& decal,bool any_emissive,NativeDecalChoice& out) {
    ds3content::DecalConstantInput in;std::memcpy(in.decal,decal.object.data(),sizeof(in.decal));std::memcpy(in.manager_400,decal.settings.data(),sizeof(in.manager_400));
    in.batch_emissive=any_emissive;in.material_present=true;std::memcpy(in.material_scale,decal.material.data()+0xD0,16);
    SceneView view;if(!BuildSceneView(scene.camera,view))return false;
    auto projection=view.projection;
    for(size_t row=0;row<4;++row)for(size_t column=0;column<4;++column){const size_t offset=256+4*(4*column+row);
        if(scene.camera_constants.written[offset/4]==1)std::memcpy(&projection[4*row+column],scene.camera_constants.bytes.data()+offset,4);}
    const auto view_projection=Multiply(view.view,projection);
    std::copy(view.view.begin(),view.view.end(),in.view.world_to_view);std::copy(projection.begin(),projection.end(),in.view.projection);
    std::copy(view_projection.begin(),view_projection.end(),in.view.view_projection);
    std::copy(scene.camera.camera_pose.begin(),scene.camera.camera_pose.end(),in.view.camera_world);
    in.view.fov=scene.camera.vertical_fov;in.view.near_plane=scene.camera.near_plane;in.view.far_plane=scene.camera.far_plane;
    in.view.aspect=scene.camera.explicit_aspect?scene.camera.explicit_aspect:float(scene.camera.width)/float(scene.camera.height);
    in.view.screen_width=scene.camera.width;in.view.screen_height=scene.camera.height;
    ds3content::DecalConstants c;if(!ds3content::ComputeDecalConstants(in,c) || c.pixel_table_index<0 || c.vertex_table_index<0)return false;
    out.pixel=static_cast<uint32_t>(c.pixel_table_index);out.vertex=static_cast<uint32_t>(c.vertex_table_index);out.inside=c.camera_inside;
    std::memcpy(out.vs_b10.data(),c.vs_b10,sizeof(c.vs_b10));std::memcpy(out.ps_b10.data(),c.ps_b10,sizeof(c.ps_b10));
    static LONG64 rows{};if(InterlockedIncrement64(&rows)<=6){char row[900]{};const auto* p=c.ps_b10;const auto* v=c.vs_b10;
        sprintf_s(row,"{\"event\":\"native_decal_constants\",\"pixel\":%d,\"vertex\":%d,\"inside\":%d,\"vs_rows\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g],\"width_scale\":[%g,%g],\"parallax\":[%g,%g,%g,%g],\"diffuse\":[%g,%g,%g,%g],\"fades\":[%g,%g,%g,%g,%g,%g]}\n",
            c.pixel_table_index,c.vertex_table_index,int(c.camera_inside),v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10],v[11],v[12],v[13],v[14],v[15],v[16],v[17],
            p[68],p[69],p[70],p[71],p[80],p[81],p[82],p[83],p[96],p[97],p[98],p[99],p[100],p[101]);Log(row);}
    return true;
}
// 50720's cube: the eight corners of 143D204A0 and the 14-index strip of 143D20480 (0,1,2,3,7,1,5,0,4,2,6,7,4,5), drawn as the
// equivalent 12-triangle list (the model draw takes triangle lists; strip winding alternates on odd triangles).
const ModelGeometryBinding* NativeDecalCube() {
    static std::mutex lock;static ModelGeometryBinding cube;static bool failed{};const std::lock_guard hold(lock);
    if(!cube.vertex_resource && !failed){
        constexpr float corners[24]={-1,1,1, 1,1,1, -1,1,-1, 1,1,-1, -1,-1,1, 1,-1,1, -1,-1,-1, 1,-1,-1};
        constexpr uint16_t strip[14]={0,1,2,3,7,1,5,0,4,2,6,7,4,5};
        std::array<uint16_t,36> list{};for(size_t i=0;i<12;++i){const size_t odd=i&1;list[3*i]=strip[i+odd];list[3*i+1]=strip[i+1-odd];list[3*i+2]=strip[i+2];}
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width=256;desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Microsoft::WRL::ComPtr<ID3D12Resource> buffer;void* mapped{};
        if(FAILED(gpu.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&buffer))) ||
           FAILED(buffer->Map(0,nullptr,&mapped))){failed=true;Log("{\"event\":\"native_decal_cube_failed\"}\n");return nullptr;}
        std::memcpy(mapped,corners,sizeof(corners));std::memcpy(static_cast<uint8_t*>(mapped)+128,list.data(),sizeof(list));buffer->Unmap(0,nullptr);
        cube.storage_generation=1;cube.vertex_resource=cube.index_resource=buffer;const auto at=buffer->GetGPUVirtualAddress();
        cube.vertices={at,sizeof(corners),12};cube.indices={at+128,static_cast<UINT>(sizeof(list)),DXGI_FORMAT_R16_UINT};cube.index_count=36;}
    return cube.vertex_resource?&cube:nullptr;
}
// The programme pair of a decal (GX shader objects of the manager's tables) as a draw programme set with its pixel interface,
// made once per pair of native programmes (a reloaded programme is a new NativeShader, so a new set).
struct NativeDecalPrograms {std::shared_ptr<const NativeShader> vertex,pixel;std::shared_ptr<const NativeModelProgramSet> set;
    std::shared_ptr<const ds3content::ShaderInterface> pixel_interface;};
const NativeDecalPrograms* NativeDecalProgramsOf(uintptr_t vertex,uintptr_t pixel) {
    std::shared_ptr<const NativeShader> vs,ps;
    {const std::lock_guard hold(shader_lock);if(const auto found=shaders.find(vertex);found!=shaders.end())vs=found->second;
        if(const auto found=shaders.find(pixel);found!=shaders.end())ps=found->second;}
    if(!vs || !ps)return nullptr;
    static std::mutex lock;static std::map<std::pair<const NativeShader*,const NativeShader*>,NativeDecalPrograms> sets;const std::lock_guard hold(lock);
    auto& entry=sets[{vs.get(),ps.get()}];
    if(!entry.set){ds3content::CpuShaderProgram v,p;v.stage=ds3content::ShaderStage::Vertex;v.model_major=5;v.bytecode.assign(vs->bytes.begin(),vs->bytes.end());
        p.stage=ds3content::ShaderStage::Pixel;p.model_major=5;p.bytecode.assign(ps->bytes.begin(),ps->bytes.end());
        try{entry.pixel_interface=ds3content::InspectShaderInterfaceShared(p);}catch(...){sets.erase({vs.get(),ps.get()});return nullptr;}
        std::vector<ds3content::ModelVertexElement> elements{{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
        entry.vertex=vs;entry.pixel=ps;entry.set=MakeNativeModelProgramSet(std::move(v),std::move(p),std::move(elements));}
    return &entry;
}
void BuildNativeDecalDraws(const std::shared_ptr<const NativeSelectedScene>& scene,NativeModelFrameInput& frame) {
    if(scene->decals.empty() || !scene->sampler_bank || !scene->default_param_row)return;
    const auto& sources=scene->decal_sources;if(!sources[0] || !sources[1] || !sources[2])return;
    const auto* cube=NativeDecalCube();if(!cube)return;
    // 51AA0: the batch's anyEmissive, some decal with +0x80 set and +0xFC > 0.
    bool any_emissive{};for(const auto& d:scene->decals){float power{};std::memcpy(&power,d.object.data()+0xFC,4);any_emissive|=d.object[0x80]!=0 && power>0.0f;}
    static std::array<LONG64,8> refusals{};static LONG64 rows{};uint64_t drawn{};
    static const bool debug=ModuleFlag(L"decal-debug.flag");
    const auto refuse=[&](size_t why){InterlockedIncrement64(&refusals[why]);};
    for(const auto& decal:scene->decals){
        NativeDecalChoice choice;if(!ProduceNativeDecalChoice(*scene,decal,any_emissive,choice)){refuse(0);continue;}
        if(choice.pixel>=decal.pixel.size() || choice.vertex>=decal.vertex.size()){refuse(1);continue;}
        const auto* programs=NativeDecalProgramsOf(decal.vertex[choice.vertex],decal.pixel[choice.pixel]);if(!programs){refuse(1);continue;}
        const auto& pixel=*programs->pixel_interface;
        NativeModelDrawInput input;input.program_set=programs->set;input.geometry=*cube;
        input.constants.push_back({ds3content::ShaderStage::Vertex,10,std::vector<uint8_t>(choice.vs_b10.begin(),choice.vs_b10.end())});
        bool complete=true;uint32_t outputs{};
        for(const auto& output:pixel.outputs)if(_stricmp(output.semantic.c_str(),"SV_Target")==0)outputs=std::max(outputs,output.semantic_index+1);
        for(const auto& resource:pixel.resources){const auto slot=resource.shader_register;
            if(resource.input_type==0){const auto declaration=std::find_if(pixel.constants.begin(),pixel.constants.end(),[&](const auto& c){return c.name==resource.name;});
                if(declaration==pixel.constants.end()){complete=false;break;}
                std::vector<uint8_t> bytes(declaration->declared_bytes);
                if(slot==10 && bytes.size()<=choice.ps_b10.size())std::memcpy(bytes.data(),choice.ps_b10.data(),bytes.size());
                else if(slot==11){if(decal.sh && bytes.size()>=48)std::memcpy(bytes.data(),decal.sh->data(),48);}   // no bank: zero rows (Reference2134 reads zero)
                else if(slot==7){std::array<uint8_t,ds3prod::draw_state_bytes> state{};std::memcpy(state.data()+0x240,decal.material.data(),decal.material.size());
                    std::vector<uint8_t> written(bytes.size()/4);ds3prod::ProduceModelParamPs(state.data(),bytes.data(),static_cast<uint32_t>(bytes.size()),written.data(),scene->default_param_row->data());}
                else{complete=false;refuse(slot==11?3:2);break;}   // b2 (the POM programmes' cbMatDynParam) is not produced yet
                input.constants.push_back({ds3content::ShaderStage::Pixel,slot,std::move(bytes)});}
            else if(resource.input_type==3){
                // 10AB80's GX sampler set: s0, s1, s2, s3, s5, s10 bank 12; s4 14; s6 0; s7 3; s8 1; s9 4; s11 13; s12 15.
                static constexpr std::array<int8_t,13> banks{12,12,12,12,14,12,0,3,1,4,12,13,15};
                for(uint32_t s=slot;s<slot+resource.count;++s){if(s>=banks.size()){complete=false;break;}
                    input.samplers.push_back({ds3content::ShaderStage::Pixel,s,scene->sampler_bank->descriptors[banks[s]]});}}
            else for(uint32_t t=slot;t<slot+resource.count;++t){
                // 198910's 18 slots: t0..t3 +0x190..+0x1A8, t4 the depth copy, t5..t12 +0x1C8..+0x200, t13 the GB0 copy, t14 the
                // GB1 copy, t15 +0x1B0, t16 +0x1B8, t17 +0x1C0.
                std::shared_ptr<const bootstrap::NativeTextureRecord> texture;
                if(t<=3)texture=decal.textures[t];else if(t==4)texture=sources[2];else if(t<=12)texture=decal.textures[t+2];
                else if(t==13)texture=sources[0];else if(t==14)texture=sources[1];else if(t<=17)texture=decal.textures[t-11];
                if(!texture){complete=false;refuse(4);break;}
                input.srvs.push_back({ds3content::ShaderStage::Pixel,t,texture->view.handle,texture->resource,texture});}
            if(!complete)break;}
        if(!complete)continue;
        input.cpu_owners.push_back(scene);
        input.sampler_bank=scene->sampler_bank.get();input.sampler_bank_generation=scene->sampler_bank->generation;
        // The observed states (DECALS-NATIVE 1): cull front, front counter-clockwise; depth read-only LESS_EQUAL; stencil read 0x01
        // EQUAL ref 1 (the receivers). Inside the box 198910 takes depth state 0 and raster mode 2 (INFERRED: no depth test, the
        // back faces culled).
        input.rasterizer.FillMode=D3D12_FILL_MODE_SOLID;input.rasterizer.CullMode=choice.inside?D3D12_CULL_MODE_BACK:D3D12_CULL_MODE_FRONT;
        input.rasterizer.FrontCounterClockwise=TRUE;input.rasterizer.DepthClipEnable=TRUE;
        auto& depth=input.depth_stencil;depth.DepthEnable=!choice.inside;depth.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;depth.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
        depth.StencilEnable=TRUE;depth.StencilReadMask=0x01;depth.StencilWriteMask=0;
        depth.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_EQUAL};depth.BackFace=depth.FrontFace;
        input.stencil_reference=1;
        // Blends (DECALS-NATIVE 1): RT0 SpecularAcc and RT3..RT6 SRC_ALPHA / INV_SRC_ALPHA with the destination alpha kept, mask
        // RGB; RT1 GB1 mask RG (its B and A hold the SSS profile); RT2 albedo by +0x208: mode A DEST_COLOR / INV_SRC_ALPHA
        // (multiply), mode B ONE / INV_SRC_ALPHA (premultiplied). mode 1 is B (OBSERVED, Reference2134), the others A
        // (INFERRED). Outputs the programme does not declare are not written (D3D11 leaves them undefined).
        uint32_t mode{};std::memcpy(&mode,decal.object.data()+0x208,4);
        input.blend.IndependentBlendEnable=TRUE;
        for(uint32_t target=0;target<7;++target){auto& rt=input.blend.RenderTarget[target];rt.BlendEnable=TRUE;
            rt.SrcBlend=D3D12_BLEND_SRC_ALPHA;rt.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;rt.BlendOp=D3D12_BLEND_OP_ADD;
            rt.SrcBlendAlpha=D3D12_BLEND_ZERO;rt.DestBlendAlpha=D3D12_BLEND_ONE;rt.BlendOpAlpha=D3D12_BLEND_OP_ADD;rt.LogicOp=D3D12_LOGIC_OP_NOOP;
            rt.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_RED|D3D12_COLOR_WRITE_ENABLE_GREEN|D3D12_COLOR_WRITE_ENABLE_BLUE;
            if(target==1)rt.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_RED|D3D12_COLOR_WRITE_ENABLE_GREEN;
            if(target==2)rt.SrcBlend=mode==1?D3D12_BLEND_ONE:D3D12_BLEND_DEST_COLOR;
            // decal-debug.flag: the albedo takes the decal's premultiplied colour outright (ONE / ZERO), so its coverage shows.
            if(target==2 && debug){rt.SrcBlend=D3D12_BLEND_ONE;rt.DestBlend=D3D12_BLEND_ZERO;}
            if(target>=outputs)rt.RenderTargetWriteMask=0;}
        input.render_target_count=7;{const auto& seven=SceneRenderer::DecalTargetFormats();std::copy(seven.begin(),seven.end(),input.render_target_formats.begin());}
        input.shape.envelope=ModelPassEnvelope::Decal7;input.depth_format=SceneRenderer::ModelDepthFormat();input.sample_mask=UINT_MAX;
        try{frame.decal_draws.push_back(PrepareNativeModelDraw(gpu.device.Get(),std::move(input)));++drawn;}
        catch(const std::exception& error){refuse(5);static LONG64 failures{};if(InterlockedIncrement64(&failures)<=8){char row[320]{};
            sprintf_s(row,"{\"event\":\"native_decal_prepare_failed\",\"what\":\"%.200s\"}\n",error.what());Log(row);}}}
    if(!frame.decal_draws.empty()){frame.decal_gb0_copy=sources[0]->resource;frame.decal_gb1_copy=sources[1]->resource;frame.decal_depth_copy=sources[2]->resource;}
    if(const auto n=InterlockedIncrement64(&rows);n<=8 || n%600==0){char row[320]{};
        sprintf_s(row,"{\"event\":\"native_decal_draws\",\"decals\":%zu,\"drawn\":%llu,\"refused\":{\"constants\":%lld,\"programme\":%lld,\"constant_buffer\":%lld,\"sh\":%lld,\"texture\":%lld,\"prepare\":%lld}}\n",
            scene->decals.size(),drawn,refusals[0],refusals[1],refusals[2],refusals[3],refusals[4],refusals[5]);Log(row);}
}
// DECALS-PAINT-READY rung 2: the paint pass's programmes of a G-buffer row (shader pass 8, table 14473CE60): the family's
// `<family>_Pnt[_Skin]` vertex programme (the effect's, else the loaded repository 4792D58 for a composed gxflvershader group)
// and the shared loose GXFlver_Common_Pnt / GXFlver_Common_Pnt_[A] pixel programme (4792BF8) by the row's pixel variant, with
// the G-buffer set's vertex elements (the Pnt VS reads the same inputs as the Gbuf one); null without them. Made once per
// (vertex, pixel, G-buffer set).
std::shared_ptr<const NativeModelProgramSet> NativePaintProgramSet(const NativeSelectedMesh& mesh,uint32_t pixel_variant,
    const std::shared_ptr<const NativeModelProgramSet>& gbuffer) {
    if(!mesh.programs || !mesh.layout || mesh.layout->type>1 || mesh.layout->instanced || !gbuffer)return {};
    static std::mutex lock;static std::array<std::shared_ptr<const NativeShader>,2> common;
    {const std::lock_guard hold(lock);auto& ps=common[pixel_variant?1:0];
        if(!ps)ps=FindLoadedProgramme(0x4792BF8,pixel_variant?L"GXFlver_Common_Pnt_[A]":L"GXFlver_Common_Pnt",0u);
        if(!ps)return {};}
    const auto& programs=*mesh.programs;auto family=programs.program_path.substr(programs.program_path.find_last_of(L"/\\")==std::wstring::npos?0:programs.program_path.find_last_of(L"/\\")+1);
    if(family.size()>4 && _wcsicmp(family.c_str()+family.size()-4,L".spx")==0)family.resize(family.size()-4);
    if(family.empty())return {};
    const auto wanted=family+L"_Pnt"+(mesh.layout->type?L"_Skin":L"");
    std::shared_ptr<const NativeShader> vs;
    if(programs.effect)for(const auto& program:programs.effect->programs){auto name=program.name;
        if(name.size()>4 && _wcsicmp(name.c_str()+name.size()-4,L".vpo")==0)name.resize(name.size()-4);
        if(program.program && _wcsicmp(name.c_str(),wanted.c_str())==0 && D3D11_SHVER_GET_TYPE(program.program->version)==1){vs=program.program;break;}}
    if(!vs)vs=FindLoadedProgramme(0x4792D58,wanted.c_str(),1u);
    if(!vs)return {};
    struct Entry {std::shared_ptr<const NativeShader> vs,ps;std::shared_ptr<const NativeModelProgramSet> gbuffer,set;};
    static std::map<std::tuple<const void*,const void*,const void*>,Entry> sets;
    const std::lock_guard hold(lock);const auto& ps=common[pixel_variant?1:0];
    auto& entry=sets[{vs.get(),ps.get(),gbuffer.get()}];
    if(!entry.set){if(sets.size()>4096){sets.clear();return {};}
        ds3content::CpuShaderProgram v,p;v.stage=ds3content::ShaderStage::Vertex;v.model_major=5;v.bytecode.assign(vs->bytes.begin(),vs->bytes.end());
        p.stage=ds3content::ShaderStage::Pixel;p.model_major=5;p.bytecode.assign(ps->bytes.begin(),ps->bytes.end());
        entry={vs,ps,gbuffer,MakeNativeModelProgramSet(std::move(v),std::move(p),gbuffer->elements)};
        // D-0193 (decal-observe.flag): each new paint programme pair's bytecode next to the trace (paint-vs-<n>.dxbc, paint-ps-<n>.dxbc).
        static const bool observe=ModuleFlag(L"decal-observe.flag");static uint32_t dumped{};
        if(observe && control && dumped<32){const auto n=dumped++;const auto directory=JournalSibling(L"");
            {std::ofstream file(directory+L"paint-vs-"+std::to_wstring(n)+L".dxbc",std::ios::binary);file.write(reinterpret_cast<const char*>(vs->bytes.data()),vs->bytes.size());}
            {std::ofstream file(directory+L"paint-ps-"+std::to_wstring(n)+L".dxbc",std::ios::binary);file.write(reinterpret_cast<const char*>(ps->bytes.data()),ps->bytes.size());}
            std::string name;for(const auto c:wanted)name+=c<128?char(c):'?';char row[200]{};
            sprintf_s(row,"{\"event\":\"native_paint_programme\",\"n\":%u,\"vs\":\"%.120s\",\"pixel_variant\":%u}\n",n,name.c_str(),pixel_variant);Log(row);}}
    return entry.set;
}
// DECALS-PAINT-READY rung 2: a G-buffer row's paint draw, derived from its draw (geometry, the bone palette VS b5, the frame
// cbSceneParam VS b8 whose bound copy the paint pass replaces, PS b2 cbMatDynParam and b3 cbMtdParam: the same declarations,
// at the same offsets, as GXFlver_Common_Pnt's): VS b0 the projector (frame), VS and PS b4 the row's instance data with
// userClipPlane zero ([1439F2490]) and mulCol the paint's (1AB370 leaves matricesData.x 0; the row's is kept, it indexes the
// row's palette), PS b10 cbPaintParam (g_PaintMaterialParamScale at +160), t0..t4 the decal's (1AC390), t32 the projector depth
// map (rung 2b, D-0193: the target's occlusion map; white, no self-occlusion, without one), s0..s3 bank row 10 (s3 row 4 without a mask), s7 row 3; the states of 1AAA70 and
// 1AB370 (raster +0x208 solid without culling, depth +0x1E0 LESS_EQUAL written, blend +0x1A8 SRC_ALPHA / INV_SRC_ALPHA with
// alpha ZERO / INV_SRC_ALPHA). Null when the row has no G-buffer binding the paint needs.
// D-0193 review: a programme's declarations, null for an empty or refused programme (no exception out of the frame prepare); and
// whether a declaration names the same resource (kind, register and name) as one of another programme's.
std::shared_ptr<const ds3content::ShaderInterface> NativePaintInterface(const ds3content::CpuShaderProgram& program) noexcept {
    try{return program.bytecode.empty()?nullptr:ds3content::InspectShaderInterfaceShared(program);}catch(...){return nullptr;}}
bool NativePaintSameResource(const ds3content::ShaderInterface* other,const ds3content::ShaderResourceDeclaration& r) noexcept {
    return other && std::any_of(other->resources.begin(),other->resources.end(),[&](const auto& x){
        return x.input_type==r.input_type && x.shader_register==r.shader_register && x.count>=r.count && x.name==r.name;});}
std::shared_ptr<const OwnedNativeModelDraw> NativePaintDraw(const NativeSelectedScene& scene,const NativeSelectedScene::PaintEvent& event,
    const std::shared_ptr<const NativeModelProgramSet>& set,const NativeModelDrawInput& g,const char*& refused) {
    NativeModelDrawInput p;p.program_set=set;p.geometry=g.geometry;
    const auto find=[&](ds3content::ShaderStage stage,uint32_t slot)->const NativeModelConstantBinding*{
        for(const auto& b:g.constants)if(b.stage==stage && b.shader_register==slot)return &b;return nullptr;};
    const auto V=ds3content::ShaderStage::Vertex,P=ds3content::ShaderStage::Pixel;
    const auto* vs_instance=find(V,4);const auto* palette=find(V,5);const auto* scene_block=find(V,8);
    const auto* matdyn=find(P,2);const auto* mtd=find(P,3);const auto* ps_instance=find(P,4);
    if(!vs_instance || !palette || !scene_block || !scene_block->frame || !matdyn || !mtd || !ps_instance){refused="constants";return {};}
    const auto instance=[&](const NativeModelConstantBinding& b){NativeModelConstantBinding out;out.stage=b.stage;out.shader_register=b.shader_register;
        out.bytes=b.Payload();if(out.bytes.size()<128)return out;std::memset(out.bytes.data()+64,0,16);std::memcpy(out.bytes.data()+112,event.mul_col.data(),16);return out;};
    {NativeModelConstantBinding view;view.stage=V;view.shader_register=0;view.bytes.assign(64,0);view.frame=true;p.constants.push_back(std::move(view));}
    p.constants.push_back(instance(*vs_instance));p.constants.push_back(*palette);p.constants.push_back(*scene_block);
    p.constants.push_back(*matdyn);p.constants.push_back(*mtd);p.constants.push_back(instance(*ps_instance));
    // Native2334-2335: some _Pnt vertex programmes declare more of the G-buffer VS's registers (b2 cbMatDynParam, b7 cbvModelParam):
    // the G-buffer row's own binding of the same register when the G-buffer programme declares the same buffer there (review: by
    // name), b2 else the row's PS b2 when its pixel programme declares cbMatDynParam there (the same block).
    const auto paint_vs=NativePaintInterface(set->vertex);
    const auto gbuffer_vs=NativePaintInterface(g.program_set->vertex),gbuffer_ps=NativePaintInterface(g.program_set->pixel);
    if(!paint_vs){refused="vertex interface";return {};}
    for(const auto& r:paint_vs->resources){if(r.input_type!=0)continue;
        if(std::any_of(p.constants.begin(),p.constants.end(),[&](const auto& b){return b.stage==V && b.shader_register==r.shader_register;}))continue;
        if(const auto* own=find(V,r.shader_register);own && NativePaintSameResource(gbuffer_vs.get(),r))p.constants.push_back(*own);
        else if(r.shader_register==2 && NativePaintSameResource(gbuffer_ps.get(),r)){auto copy=*matdyn;copy.stage=V;p.constants.push_back(std::move(copy));}
        else{refused="vertex constant";return {};}}
    {NativeModelConstantBinding paint;paint.stage=P;paint.shader_register=10;paint.bytes.assign(208,0);std::memcpy(paint.bytes.data()+160,event.param_scale.data(),16);p.constants.push_back(std::move(paint));}   // Native2331: the RDEF declares 208 bytes (FC_PaintEraseColor +176, FC_PaintFadeParam +192, unread by Common_Pnt: zero)
    for(const auto& b:p.constants)if(b.shader_register==4 && b.Payload().size()<128){refused="instance";return {};}   // VS and PS b4
    std::shared_ptr<const bootstrap::NativeTextureRecord> white;
    for(const auto& [name,texture]:scene.common_images->system)if(texture && _wcsicmp(name.c_str(),L"SYSTEX_WHITE")==0){white=texture;break;}
    if(!white){refused="white";return {};}
    const std::array<uint32_t,5> registers{0,1,2,3,4};
    for(size_t t=0;t<5;++t){const auto& texture=event.textures[t];p.srvs.push_back({P,registers[t],texture->view.handle,texture->resource,texture});}
    {const auto& t32=event.occlusion?event.occlusion:white;p.srvs.push_back({P,32,t32->view.handle,t32->resource,t32});}   // D-0193 rung 2b: the projector depth map (white: no self-occlusion)
    const auto& bank=scene.sampler_bank->descriptors;
    for(uint32_t s=0;s<4;++s)p.samplers.push_back({P,s,bank[s==3 && !event.masked?4:10]});
    p.samplers.push_back({P,7,bank[3]});
    p.cpu_owners=g.cpu_owners;
    p.sampler_bank=scene.sampler_bank.get();p.sampler_bank_generation=scene.sampler_bank->generation;
    p.rasterizer.FillMode=D3D12_FILL_MODE_SOLID;p.rasterizer.CullMode=D3D12_CULL_MODE_NONE;p.rasterizer.DepthClipEnable=TRUE;
    p.depth_stencil.DepthEnable=TRUE;p.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;p.depth_stencil.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
    {auto& rt=p.blend.RenderTarget[0];rt.BlendEnable=TRUE;rt.SrcBlend=D3D12_BLEND_SRC_ALPHA;rt.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;rt.BlendOp=D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha=D3D12_BLEND_ZERO;rt.DestBlendAlpha=D3D12_BLEND_INV_SRC_ALPHA;rt.BlendOpAlpha=D3D12_BLEND_OP_ADD;rt.LogicOp=D3D12_LOGIC_OP_NOOP;
        rt.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;}
    p.render_target_count=2;p.render_target_formats[0]=SceneRenderer::paint_formats[0];p.render_target_formats[1]=SceneRenderer::paint_formats[1];
    p.shape.envelope=ModelPassEnvelope::Paint2;p.depth_format=DXGI_FORMAT_D32_FLOAT;p.sample_mask=UINT_MAX;
    try{return PrepareNativeModelDraw(gpu.device.Get(),std::move(p));}
    catch(const std::exception& error){refused="prepare";static LONG64 failures{};if(InterlockedIncrement64(&failures)<=8){char row[320]{};
        sprintf_s(row,"{\"event\":\"native_paint_prepare_failed\",\"what\":\"%.200s\"}\n",error.what());Log(row);}return {};}
}
// D-0193 (DECALS-PAINT-READY rung 2b): a G-buffer row's projector depth draw, the original's depth pass (shader pass 2, 1AAA70) into
// the target's D16 512 x 512 map: the mesh's Dpt programme (DptA for an alpha-tested pixel variant, recipes 20 / 21) on the row's
// geometry, every register it declares taken from the row's own G-buffer binding of the same stage and register (the same
// mesh, instance and material), VS b0 a frame block the paint pass fills (F at +64, as the casters' cascade blocks), VS b4 with
// userClipPlane zero; depth LESS_EQUAL written, cull none, no colour target. Null when a declared register has no such binding.
std::shared_ptr<const OwnedNativeModelDraw> NativePaintDepthDraw(const NativeSelectedMesh& mesh,uint32_t pixel_variant,const NativeModelDrawInput& g,const char*& refused) {
    const size_t recipe=NativeRecipe(2,pixel_variant);
    if(!mesh.programs || !g.program_set || mesh.programs->recipes.at(recipe).status!=ds3content::ModelProgramStatus::Resolved){refused="depth programme";return {};}
    NativeModelDrawInput p;p.geometry=g.geometry;
    try{p.program_set=NativeProgramSetOf(*mesh.programs,recipe,g.program_set->elements);}catch(...){refused="depth programme";return {};}
    const auto V=ds3content::ShaderStage::Vertex;
    // Review: every copied register is the same buffer, texture or sampler (by name) in the G-buffer programme of that stage.
    const std::array<std::shared_ptr<const ds3content::ShaderInterface>,2> interfaces{NativePaintInterface(p.program_set->vertex),NativePaintInterface(p.program_set->pixel)};
    const std::array<std::shared_ptr<const ds3content::ShaderInterface>,2> gbuffer{NativePaintInterface(g.program_set->vertex),NativePaintInterface(g.program_set->pixel)};
    if(!interfaces[0] || (!p.program_set->pixel.bytecode.empty() && !interfaces[1])){refused="depth interface";return {};}
    for(uint32_t s=0;s<2;++s){if(!interfaces[s])continue;const auto stage=s?ds3content::ShaderStage::Pixel:V;
        for(const auto& r:interfaces[s]->resources){
            if(r.input_type==0 && !s && r.shader_register==0){uint32_t bytes{128};for(const auto& c:interfaces[s]->constants)if(c.name==r.name)bytes=std::max(bytes,c.declared_bytes);
                if(bytes>256){refused="depth view extent";return {};}   // the paint pass uploads 256 bytes (EncodeNativePaint)
                NativeModelConstantBinding view;view.stage=V;view.shader_register=0;view.bytes.assign(bytes,0);view.frame=true;p.constants.push_back(std::move(view));continue;}
            if(!NativePaintSameResource(gbuffer[s].get(),r)){refused="depth binding";return {};}
            if(r.input_type==0){
                const auto own=std::find_if(g.constants.begin(),g.constants.end(),[&](const auto& b){return b.stage==stage && b.shader_register==r.shader_register;});
                if(own==g.constants.end()){refused="depth constant";return {};}
                auto copy=*own;if(r.shader_register==4 && copy.Payload().size()>=80){copy.bytes=copy.Payload();copy.shared.reset();std::memset(copy.bytes.data()+64,0,16);}
                p.constants.push_back(std::move(copy));}
            else if(r.input_type==3){for(uint32_t at=r.shader_register;at<r.shader_register+r.count;++at){
                const auto own=std::find_if(g.samplers.begin(),g.samplers.end(),[&](const auto& b){return b.stage==stage && b.shader_register==at;});
                if(own==g.samplers.end()){refused="depth sampler";return {};}p.samplers.push_back(*own);}}
            else for(uint32_t at=r.shader_register;at<r.shader_register+r.count;++at){
                const auto own=std::find_if(g.srvs.begin(),g.srvs.end(),[&](const auto& b){return b.stage==stage && b.shader_register==at;});
                if(own==g.srvs.end()){refused="depth texture";return {};}p.srvs.push_back(*own);}}}
    p.cpu_owners=g.cpu_owners;p.sampler_bank=g.sampler_bank;p.sampler_bank_generation=g.sampler_bank_generation;
    p.rasterizer.FillMode=D3D12_FILL_MODE_SOLID;p.rasterizer.CullMode=D3D12_CULL_MODE_NONE;p.rasterizer.DepthClipEnable=TRUE;
    p.depth_stencil.DepthEnable=TRUE;p.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;p.depth_stencil.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
    p.render_target_count=0;p.render_target_formats={};p.shape.envelope=ModelPassEnvelope::DepthOnly;p.depth_format=DXGI_FORMAT_D16_UNORM;p.sample_mask=UINT_MAX;
    try{return PrepareNativeModelDraw(gpu.device.Get(),std::move(p));}
    catch(const std::exception& error){refused="depth prepare";static LONG64 failures{};if(InterlockedIncrement64(&failures)<=8){char row[320]{};
        sprintf_s(row,"{\"event\":\"native_paint_depth_failed\",\"what\":\"%.200s\"}\n",error.what());Log(row);}return {};}
}
// I-0105 (BLOOD.md): the erase programmes of a G-buffer row's mesh, pass 10's: VS <family>_PntCol[_Skin] (the second UV set's
// integer texel as position over 1024, depth 0, COLOR1 through) and PS GXFlver_Common_PntCol, on the row's vertex elements. Null
// when the family has no _PntCol programme (no erase there).
std::shared_ptr<const NativeModelProgramSet> NativeEraseProgramSet(const NativeSelectedMesh& mesh,const std::shared_ptr<const NativeModelProgramSet>& gbuffer) {
    if(!mesh.programs || !mesh.layout || mesh.layout->type>1 || mesh.layout->instanced || !gbuffer)return {};
    static std::mutex lock;static std::shared_ptr<const NativeShader> ps;
    {const std::lock_guard hold(lock);if(!ps)ps=FindLoadedProgramme(0x4792BF8,L"GXFlver_Common_PntCol",0u);if(!ps)return {};}
    const auto& programs=*mesh.programs;auto family=programs.program_path.substr(programs.program_path.find_last_of(L"/\\")==std::wstring::npos?0:programs.program_path.find_last_of(L"/\\")+1);
    if(family.size()>4 && _wcsicmp(family.c_str()+family.size()-4,L".spx")==0)family.resize(family.size()-4);
    if(family.empty())return {};
    const auto wanted=family+L"_PntCol"+(mesh.layout->type?L"_Skin":L"");
    std::shared_ptr<const NativeShader> vs;
    if(programs.effect)for(const auto& program:programs.effect->programs){auto name=program.name;
        if(name.size()>4 && _wcsicmp(name.c_str()+name.size()-4,L".vpo")==0)name.resize(name.size()-4);
        if(program.program && _wcsicmp(name.c_str(),wanted.c_str())==0 && D3D11_SHVER_GET_TYPE(program.program->version)==1){vs=program.program;break;}}
    if(!vs)vs=FindLoadedProgramme(0x4792D58,wanted.c_str(),1u);
    if(!vs)return {};
    struct Entry {std::shared_ptr<const NativeShader> vs;std::shared_ptr<const NativeModelProgramSet> gbuffer,set;};
    static std::map<std::pair<const void*,const void*>,Entry> sets;
    const std::lock_guard hold(lock);auto& entry=sets[{vs.get(),gbuffer.get()}];
    if(!entry.set){if(sets.size()>4096){sets.clear();return {};}
        ds3content::CpuShaderProgram v,p;v.stage=ds3content::ShaderStage::Vertex;v.model_major=5;v.bytecode.assign(vs->bytes.begin(),vs->bytes.end());
        p.stage=ds3content::ShaderStage::Pixel;p.model_major=5;p.bytecode.assign(ps->bytes.begin(),ps->bytes.end());
        entry={vs,gbuffer,MakeNativeModelProgramSet(std::move(v),std::move(p),gbuffer->elements)};}
    return entry.set;
}
// I-0105 (BLOOD.md, 1AAF90 read in GhidraMCP): a G-buffer row's erase draw: its geometry with the erase programmes, PS b10
// cbPaintParam (208 bytes, FC_PaintEraseColor +176 and FC_PaintFadeParam +192 the event's; the programmes declare nothing else),
// into the paint target's two textures. States: raster [[GX+0x2A0]+0x208] (solid, no culling), depth +0x1E0 (LESS_EQUAL written: the
// VS's depth 0 always passes), blend [[GXRS+0x4DD0]+0x320] (140072380: every target SRC_ALPHA / INV_SRC_ALPHA ADD, alpha
// INV_DEST_ALPHA / ONE ADD, all channels): each texel lerps toward (0,0,0,1) by the pixel's alpha.
std::shared_ptr<const OwnedNativeModelDraw> NativeEraseDraw(const NativeSelectedScene::PaintEvent& event,const std::shared_ptr<const NativeModelProgramSet>& set,
    const NativeModelDrawInput& g,const char*& refused) {
    NativeModelDrawInput p;p.program_set=set;p.geometry=g.geometry;
    const auto vertex=NativePaintInterface(set->vertex),pixel=NativePaintInterface(set->pixel);
    if(!vertex || !pixel){refused="interface";return {};}
    if(!vertex->resources.empty() || pixel->resources.size()!=1 || pixel->resources[0].input_type!=0 || pixel->resources[0].shader_register!=10){refused="declarations";return {};}
    {NativeModelConstantBinding paint;paint.stage=ds3content::ShaderStage::Pixel;paint.shader_register=10;paint.bytes.assign(208,0);
        std::memcpy(paint.bytes.data()+176,event.erase_block.data(),32);p.constants.push_back(std::move(paint));}
    p.cpu_owners=g.cpu_owners;p.sampler_bank=g.sampler_bank;p.sampler_bank_generation=g.sampler_bank_generation;
    p.rasterizer.FillMode=D3D12_FILL_MODE_SOLID;p.rasterizer.CullMode=D3D12_CULL_MODE_NONE;p.rasterizer.DepthClipEnable=TRUE;
    p.depth_stencil.DepthEnable=TRUE;p.depth_stencil.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;p.depth_stencil.DepthFunc=D3D12_COMPARISON_FUNC_LESS_EQUAL;
    {auto& rt=p.blend.RenderTarget[0];rt.BlendEnable=TRUE;rt.SrcBlend=D3D12_BLEND_SRC_ALPHA;rt.DestBlend=D3D12_BLEND_INV_SRC_ALPHA;rt.BlendOp=D3D12_BLEND_OP_ADD;
        rt.SrcBlendAlpha=D3D12_BLEND_INV_DEST_ALPHA;rt.DestBlendAlpha=D3D12_BLEND_ONE;rt.BlendOpAlpha=D3D12_BLEND_OP_ADD;rt.LogicOp=D3D12_LOGIC_OP_NOOP;
        rt.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;}
    p.render_target_count=2;p.render_target_formats[0]=SceneRenderer::paint_formats[0];p.render_target_formats[1]=SceneRenderer::paint_formats[1];
    p.shape.envelope=ModelPassEnvelope::Paint2;p.depth_format=DXGI_FORMAT_D32_FLOAT;p.sample_mask=UINT_MAX;
    try{return PrepareNativeModelDraw(gpu.device.Get(),std::move(p));}
    catch(const std::exception& error){refused="prepare";static LONG64 failures{};if(InterlockedIncrement64(&failures)<=8){char row[320]{};
        sprintf_s(row,"{\"event\":\"native_erase_prepare_failed\",\"what\":\"%.200s\"}\n",error.what());Log(row);}return {};}
}
// I-0100 T0 (popin-observe.flag, POPIN-STUDY §4 M1): the main view's models by (entity, model): drawn (one of its rows other
// than the shadow and velocity ones drawn), else why not: not ready (its rows prepared, none drawn: D-0197's delay or an
// incomplete row), no main row, refused at admission (its code: zero_fade, missing inputs, instance streams, missing geometry,
// no meshes, another failure) or absent (not walked: outside the view). One popin_native row each time a model starts or
// stops being drawn, compared with the previous main prepare (a scene presented again is skipped).
void PopinNativeFrame(const NativeSelectedScene& scene,const std::vector<std::pair<size_t,bool>>& rows) noexcept {
    try{struct Drawn {std::string name;float centre[3]{};uintptr_t entity{};};
        static std::mutex lock;static std::unordered_map<uint64_t,Drawn> previous;static uint64_t last_scene{},frames{},on{},off{};
        const std::lock_guard hold(lock);if(scene.uid==last_scene)return;last_scene=scene.uid;++frames;   // the scene, not its frame serial (review L2)
        static const char* const whys[]{"drawn","zero_fade","missing_inputs","instance_streams","missing_geometry","no_meshes","failed","nonfinite",
            "","","not_ready","no_main_row","absent"};
        std::vector<uint8_t> state(scene.models.size());   // 0 no main row, 1 not ready, 2 drawn
        for(const auto& [model,drawn]:rows)if(model<state.size()){auto& s=state[model];if(drawn)s=2;else if(!s)s=1;}
        const auto key_of=[](uintptr_t entity,uintptr_t model){return uint64_t(entity)^(uint64_t(model)<<1);};
        std::unordered_map<uint64_t,uint8_t> now;now.reserve(scene.models.size()+scene.popin_refused.size());
        std::unordered_map<uint64_t,size_t> index;index.reserve(scene.models.size());
        const auto row=[&](bool drawn,uintptr_t entity,const std::string& name,const PopinPlace& p,uint8_t why,const NativeSelectedModel* m){
            // The weights the admission read (the track's and the geometry's, ModelFadeState), when the model is in the scene.
            float opacity{-1.f},weights[2]{p.weights[0],p.weights[1]};int dither{-1};uint32_t intents{};
            if(m){weights[0]=m->fade.normal.weight;weights[1]=m->fade.geometry_weight;intents=m->geometry_fades.count;
                if(intents){const auto& f=m->geometry_fades.intents[0];opacity=f.opacity;dither=f.fade_enabled && scene.category_scalar && f.opacity<*scene.category_scalar?1:0;}}
            char text[520]{};sprintf_s(text,"{\"event\":\"popin_native\",\"serial\":%llu,\"on\":%d,\"entity\":\"%llx\",\"model\":\"%s\",\"why\":\"%s\",\"centre\":[%.2f,%.2f,%.2f],\"d\":%.2f,\"w\":[%.3f,%.3f],\"o\":%.3f,\"dither\":%d,\"intents\":%u}\n",
                static_cast<unsigned long long>(scene.serial),drawn?1:0,static_cast<unsigned long long>(entity),name.c_str(),whys[why],p.centre[0],p.centre[1],p.centre[2],p.d,weights[0],weights[1],opacity,dither,intents);Log(text);};
        for(size_t i=0;i<scene.models.size();++i){const auto& m=scene.models[i];
            const auto key=key_of(m.entity,m.model);const uint8_t why=state[i]==2?0:state[i]==1?10:11;
            if(const auto [at,fresh]=now.try_emplace(key,why);!fresh && why<at->second)at->second=why;   // a model listed twice: its best
            index.try_emplace(key,i);
            // A model drawn with a fade intent's opacity under 1 (as the original's popin_reference_fade): one row a frame.
            if(why==0 && m.geometry_fades.count && m.geometry_fades.intents[0].opacity<0.999f){const auto& f=m.geometry_fades.intents[0];
                const auto place=PopinPlaceAt(m.placement.data(),0);char text[320]{};
                sprintf_s(text,"{\"event\":\"popin_native_fade\",\"serial\":%llu,\"entity\":\"%llx\",\"model\":\"%s\",\"o\":%.3f,\"dither\":%d,\"centre\":[%.2f,%.2f,%.2f],\"d\":%.2f}\n",
                    static_cast<unsigned long long>(scene.serial),static_cast<unsigned long long>(m.entity),PopinName(m.model).c_str(),f.opacity,f.fade_enabled && scene.category_scalar && f.opacity<*scene.category_scalar?1:0,
                    place.centre[0],place.centre[1],place.centre[2],place.d);Log(text);}
            if(why==0 && !previous.contains(key)){const auto place=PopinPlaceAt(m.placement.data(),0);Drawn d{PopinName(m.model)};std::memcpy(d.centre,place.centre,sizeof d.centre);d.entity=m.entity;
                row(true,m.entity,d.name,place,0,&m);++on;previous.emplace(key,std::move(d));}}
        for(const auto& r:scene.popin_refused)now.try_emplace(key_of(r.entity,r.model),r.code<=6?r.code:uint8_t(6));
        // I-0100 tranche 2 (T2a: o000499, o319250, m31_00_00_00_009400 pass their piece's gates and are never drawn): a model refused
        // at admission without having been drawn, one popin_native_refused row when its refusal starts or changes.
        {static std::unordered_map<uint64_t,uint8_t> refused;std::unordered_map<uint64_t,uint8_t> still;still.reserve(scene.popin_refused.size());
            for(const auto& r:scene.popin_refused){const auto key=key_of(r.entity,r.model);const uint8_t code=r.code<=6?r.code:uint8_t(6);
                if(previous.contains(key) || !still.try_emplace(key,code).second)continue;
                if(const auto found=refused.find(key);found!=refused.end() && found->second==code)continue;
                uintptr_t transforms{},matrices{};float matrix[12]{};
                if(Read(r.entity+0x6F8,&transforms,8) && transforms && Read(transforms+0x60,&matrices,8) && matrices)Read(matrices,matrix,sizeof matrix);   // an item's draw matrix (a draw entity's reads as zero)
                const auto place=PopinPlaceAt(matrix,0);char text[320]{};
                sprintf_s(text,"{\"event\":\"popin_native_refused\",\"serial\":%llu,\"entity\":\"%llx\",\"model\":\"%s\",\"why\":\"%s\",\"centre\":[%.2f,%.2f,%.2f],\"d\":%.2f}\n",
                    static_cast<unsigned long long>(scene.serial),static_cast<unsigned long long>(r.entity),PopinName(r.model).c_str(),whys[code],place.centre[0],place.centre[1],place.centre[2],place.d);Log(text);}
            refused.swap(still);}
        for(auto it=previous.begin();it!=previous.end();){const auto found=now.find(it->first);const uint8_t why=found==now.end()?12:found->second;
            if(!why){++it;continue;}
            PopinPlace place;std::memcpy(place.centre,it->second.centre,sizeof place.centre);place.d=PopinDistance(place.centre);
            const auto at=index.find(it->first);   // the not-ready model's fade, when it is still in the scene
            row(false,it->second.entity,it->second.name,place,why,at==index.end()?nullptr:&scene.models[at->second]);++off;it=previous.erase(it);}
        // I-0115 (Thomas, 2026-11-01: a Silver Knight 2-4 m away at the Anor Londo stairs vanishing and coming back from one frame to
        // the next, never seen in the original): each character model within kPopinNear metres of the camera, one popin_native_near
        // row when its state changes: drawn or why not, its ordinary category (2 deferred, 0x21, 0x29: the fade group's +0x84/+0x85/
        // +0x87 and +0xDC against the scalar, 09_model_admission) with those words, its fade intent's opacity; « absent » when it
        // leaves the scene while near, « far » when it moves beyond. The original's same words: popin_reference_near (04c).
        {static std::unordered_map<uint64_t,std::pair<uint64_t,Drawn>> near_seen;std::unordered_set<uint64_t> near_now;
            const auto near_row=[&](const char* why,const Drawn& d,float distance,uint32_t category,const BYTE* k,float dc,float o){char text[400]{};
                sprintf_s(text,"{\"event\":\"popin_native_near\",\"serial\":%llu,\"entity\":\"%llx\",\"model\":\"%s\",\"why\":\"%s\",\"d\":%.2f,\"category\":%u,\"k\":[%u,%u,%u],\"dc\":%.3f,\"o\":%.3f}\n",
                    static_cast<unsigned long long>(scene.serial),static_cast<unsigned long long>(d.entity),d.name.c_str(),why,distance,category,k[0],k[1],k[2],dc,o);Log(text);};
            for(size_t i=0;i<scene.models.size();++i){const auto& m=scene.models[i];if(!m.character || !m.group)continue;
                const auto place=PopinPlaceAt(m.placement.data(),0);const auto key=key_of(m.entity,m.model);
                if(!(place.d>=0.f && place.d<kPopinNear))continue;
                if(!near_now.insert(key).second)continue;
                BYTE k[3]{};float dc{-1.f};Read(m.group+0x84,&k[0],1);Read(m.group+0x85,&k[1],1);Read(m.group+0x87,&k[2],1);Read(m.group+0xDC,&dc,4);
                const float o=m.geometry_fades.count?m.geometry_fades.intents[0].opacity:-1.f;
                const uint8_t why=state[i]==2?0:state[i]==1?10:11;const uint32_t category=m.ordinary_category.value_or(0);
                const uint64_t code=uint64_t(why)|uint64_t(category&0xFF)<<8|uint64_t(k[0])<<16|uint64_t(k[1])<<24|uint64_t(k[2])<<32|
                    uint64_t(uint8_t(std::lround(std::clamp(dc,-1.f,2.f)*50.f)))<<40|uint64_t(uint8_t(std::lround(std::clamp(o,-1.f,2.f)*50.f)))<<48;
                auto [at,fresh]=near_seen.try_emplace(key);
                if(fresh){at->second.second.name=PopinName(m.model);at->second.second.entity=m.entity;}
                else if(at->second.first==code)continue;
                at->second.first=code;near_row(whys[why],at->second.second,place.d,category,k,dc,o);}
            for(auto it=near_seen.begin();it!=near_seen.end();){if(near_now.contains(it->first)){++it;continue;}
                const bool listed=index.contains(it->first);const BYTE none[3]{};
                near_row(listed?"far":"absent",it->second.second,-1.f,0,none,-1.f,-1.f);it=near_seen.erase(it);}}
        if(frames<=3 || frames%600==0){char text[240]{};sprintf_s(text,"{\"event\":\"popin_native_frame\",\"serial\":%llu,\"frames\":%llu,\"drawn\":%zu,\"refused\":%zu,\"on\":%llu,\"off\":%llu}\n",
            static_cast<unsigned long long>(scene.serial),static_cast<unsigned long long>(frames),previous.size(),scene.popin_refused.size(),
            static_cast<unsigned long long>(on),static_cast<unsigned long long>(off));Log(text);}}
    catch(...){}
}
// I-0100 tranche 8 (popin-observe.flag, POPIN-STUDY M2): the main view around a load, one popin_load_frame row a prepare after the
// loading page's window was asked to close or after a gap in the presentations (an empty scene returns before this point), for the
// next 600 prepares: the rows prepared and drawn, the rows refused for a texture not resident and the mask of missing constants
// (D-0197's delay), the models drawn and not ready, DS3's texture creations so far and the time since the close. Main prepares
// are serialised (inline under gpu_submissions or one worker job); the lock keeps the statics safe if that ever changes.
void PopinLoadFrame(const NativeSelectedScene& scene,uint64_t presentation,size_t draws,uint32_t missing_textures,uint32_t missing_constant_mask,
    const std::vector<std::pair<size_t,bool>>& rows) noexcept {
    static std::mutex lock;static uint64_t last_presentation{},remaining{};static int64_t last_closed{};
    const std::lock_guard hold(lock);
    const auto closed=popin_page_closed.load(std::memory_order_relaxed);
    if(presentation>last_presentation+2 || closed!=last_closed)remaining=600;
    last_presentation=presentation;last_closed=closed;if(!remaining)return;--remaining;
    std::vector<uint8_t> state(scene.models.size());size_t drawn{},not_ready{};
    for(const auto& [model,on]:rows)if(model<state.size()){auto& m=state[model];if(on)m=2;else if(!m)m=1;}
    for(const auto m:state){drawn+=m==2;not_ready+=m==1;}
    static const double tick_ms=[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return 1000.0/double(f.QuadPart);}();
    LARGE_INTEGER now{};QueryPerformanceCounter(&now);char text[400]{};
    sprintf_s(text,"{\"event\":\"popin_load_frame\",\"serial\":%llu,\"presentation\":%llu,\"page_ms\":%.1f,\"rows\":%zu,\"draws\":%zu,\"missing_textures\":%u,\"missing_constant_mask\":%u,\"models\":%zu,\"drawn\":%zu,\"not_ready\":%zu,\"refused\":%zu,\"textures\":%llu}\n",
        static_cast<unsigned long long>(scene.serial),static_cast<unsigned long long>(presentation),closed?double(now.QuadPart-closed)*tick_ms:-1.0,
        scene.prepared_meshes.size(),draws,missing_textures,missing_constant_mask,scene.models.size(),drawn,not_ready,scene.popin_refused.size(),
        static_cast<unsigned long long>(popin_texture_creations.load(std::memory_order_relaxed)));Log(text);
}
}  // namespace
