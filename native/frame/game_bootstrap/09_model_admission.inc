// Model admission in the selection walks: the admission marks, the shadow walk gate, the candidate admission and the
// selected objects. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier
// part (08_model_programmes.inc before it).
#pragma once
#include "08_model_programmes.inc"
namespace {
void AdmitNativeModelCandidateBody(NativeSelectionVisitor* visitor,const NativeSelectionCandidate& c);
// m45 collect cost (light-observe.flag, or admission-marks.flag alone since I-0078): the model admission's sections, QPC ticks
// between marks, summed over the main walk and logged with native_main_walk_outcomes (sections 0-7: fade, inputs, bindings,
// instances, carried tables, record check, reuse plans, engine bytes; shadow-walk admissions on the same thread are counted with
// the next collect).
bool NativeAdmissionMarksOn() noexcept {static const bool on=ModuleFlag(L"light-observe.flag") || ModuleFlag(L"admission-marks.flag");return on;}
struct NativeAdmissionMarks {
    LARGE_INTEGER last{};bool on{};std::array<volatile LONG64,12> ticks{},counts{};
    void Start() noexcept {on=NativeAdmissionMarksOn();if(on)QueryPerformanceCounter(&last);}
    void Mark(unsigned section) noexcept {if(!on)return;LARGE_INTEGER now{};QueryPerformanceCounter(&now);
        InterlockedAdd64(&ticks[section],now.QuadPart-last.QuadPart);InterlockedIncrement64(&counts[section]);last=now;}
};
thread_local NativeAdmissionMarks admission_marks;
std::array<volatile LONG64,12> native_admission_ticks{},native_admission_counts{};   // collected from the threads' marks
void FlushNativeAdmissionMarks() noexcept {for(size_t i=0;i<12;++i){InterlockedAdd64(&native_admission_ticks[i],InterlockedExchange64(&admission_marks.ticks[i],0));
    InterlockedAdd64(&native_admission_counts[i],InterlockedExchange64(&admission_marks.counts[i],0));}}
// I-0071 point 3: the outcome code of this thread's last admission (see NativeDeferredCandidate), and, on an admission
// worker, where its admitted model goes instead of the scene: I-0077, the slot of its candidate (one per candidate, sized
// before the workers start: no list grows or moves its models while they run).
thread_local uint8_t native_admission_code{};
thread_local std::optional<NativeSelectedModel>* native_admission_sink{};thread_local size_t native_admission_index{};
// I-0097 tranche 2 (crossing-observe.flag): this thread's admissions made afresh (no record reused), read by the deferred admission.
thread_local uint64_t native_fresh_admissions{};
bool NativeCrossingObserve() noexcept {static const bool on=ModuleFlag(L"crossing-observe.flag");return on;}
// E03 (I-0105): a model particle's fade state. It has no entity: one full track (weight 1, no interpolation) at the LOD its draw asks
// (636F0's args+0x18, the LOD 63CB0 draws), with the draw state's own opacity, flags and fade bytes (+0x124, +0x98, +0x88, +0x89),
// which ReadModelFadeStateAt reads from an entity's group.
ingress::ModelFadeState NativeEffectModelFade(const NativeEffectModel& effect) noexcept {
    ingress::ModelFadeState fade;const auto& state=effect.submission.state;
    fade.normal.weight=1.f;fade.normal.current_variant=fade.normal.previous_variant=static_cast<int32_t>(effect.submission.lod);
    fade.alternate=fade.normal;fade.geometry_weight=1.f;
    std::memcpy(&fade.group_opacity,state.data()+0x124,4);std::memcpy(&fade.group_flags,state.data()+0x98,4);
    fade.fade_enabled=state[0x88]!=0;fade.alternate_fade_enabled=state[0x89]!=0;
    return fade;
}
// I-0113 gap 17 (STUDY-TRANSLUCENT-CHAMELEON §5, VERIFIED on 140200840, slot 1 of GXFlverMaterialParamBlendMode, and its table 14473E5C0): inside
// the mesh loop a model's draw binds the blend its material's blend-mode parameter chooses, whatever the effect context bound before
// (i113-t9-crystal-oracle2: set by 15220 from there for every crystal and flask draw). With the arguments' flags f (+0x200) and the
// group's kind +0x84 at -1 (29ADF0 sets it; 1A86C0 then puts the group's +0x85, the FFX command's +0x30 byte, in +0x204 as r), the
// index is the table's [r][material mode], then bit 16 +15, else bit 17 +20, else bit 12 +10, else bit 21 (29ADF0's from the
// command's +0x34) +0x33 (an index 0 takes 20 with bit 17, 0x33 with bit 21), bit 15 without bit 21 +0x19, bit 19 0x32; the bank
// entry is +8 + 8 x index (the effect bank: 0x10 the alpha blend the crystals and the flask draw with). 0 when the rule does not
// apply (a kind set: 1A86C0 binds its own bank's entry; an index past the table's 36 words). The table's indices: 0 off, 1 alpha,
// 2 multiply, 3 additive, 4 reverse subtract; it is read flat (r x 6 + m), as 140200840 reads it, so a mode past 5 reads the next
// row. 140200840's own depth push (0x1E8 with bit 16, 17 or 11) is not modelled: Begin sets the depth after it.
uint32_t NativeMaterialBlendToken(uint8_t kind,uint8_t row,uint32_t f,uint32_t material_mode) noexcept {
    if(kind!=0xFF || (f&1))return 0;
    static constexpr uint8_t table[36]{0,0,1,2,3,4, 1,1,1,2,3,4, 2,2,2,2,2,2, 3,3,3,3,3,3, 4,4,4,4,4,4, 0,0,0,0,0,0};   // 14473E5C0
    const int r=static_cast<int8_t>(row);if(r<0 || material_mode>=36 || static_cast<uint32_t>(r)*6+material_mode>=36)return 0;
    uint32_t i=table[static_cast<uint32_t>(r)*6+material_mode];
    if(!i){if(f>>17&1)i=0x14;else if(f>>21&1)i=0x33;}
    else if(f>>16&1)i+=0xF;else if(f>>17&1)i+=0x14;else if(f>>12&1)i+=10;else if(f>>21&1)i+=0x33;
    if((f>>15&1) && !(f>>21&1))i+=0x19;
    if(f>>19&1)i=0x32;
    return 8+8*i;
}
uint32_t NativeEffectModelBlend(const NativeEffectModel& effect,uint32_t material_mode) noexcept {
    const auto& s=effect.submission;return NativeMaterialBlendToken(s.state[0x84],s.state[0x85],s.arg_flags,material_mode);}
// I-0113 tranche 10b (STUDY-TRANSLUCENT-CHAMELEON §1 and §5): an ordinary model's forward row takes 1A86C0's kind blend when its
// group's +0x84 is a kind k (the device's table [[GXRS+4DD0]+2F0 + 8 k], i113-t10-spear-video-original's reference_kind_blends:
// k 0..3 the effect bank's +0x08..+0x20, k 4 SRC_ALPHA / ZERO reverse subtract), else its material's blend-mode parameter with the
// group's +0x85 as the row (NativeMaterialBlendToken); 5A120's preparation 0 sets none of the flags the rule reads (bit 20, the fade
// dither, aside; bits 14 and 15 from the pass +0x714, clear natively as for the E03 rows). False when neither resolves.
bool NativeOrdinaryForwardBlend(uint8_t kind,uint8_t row,uint32_t material_mode,D3D12_BLEND_DESC& out) noexcept {
    if(kind==4){out={};auto& t=out.RenderTarget[0];t.BlendEnable=TRUE;t.SrcBlend=D3D12_BLEND_SRC_ALPHA;t.DestBlend=D3D12_BLEND_ZERO;
        t.BlendOp=D3D12_BLEND_OP_REV_SUBTRACT;t.SrcBlendAlpha=D3D12_BLEND_ONE;t.DestBlendAlpha=D3D12_BLEND_ZERO;t.BlendOpAlpha=D3D12_BLEND_OP_ADD;
        t.LogicOp=D3D12_LOGIC_OP_NOOP;t.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;for(int i=1;i<8;++i)out.RenderTarget[i]=t;return true;}
    const uint32_t token=kind<4?0x08u+8u*kind:NativeMaterialBlendToken(kind,row,0,material_mode);
    return token && ds3native::NativeEffectBankBlend(token,out);
}
// I-0113 tranche 14 (a world object's region IBL): 5C0E0 runs 66C80/66DB0 and 4E2A0 on its copy of the group when the class term
// meets the model's +0x220: the OR of its preparation-0 rows' class masks (in the main view 0x29's 0x12, 0x1F's 0x80 for a model
// with +0x224 bit 12; category 0x29 the generic 0xFFFF; OBSERVED terms 0x12 and 0x92, i113-t14-al-original). Natively +0x220 is
// not the union (0xFFFF on every part, i113-t13-*-native) and the union of the meshes' classes is known only after the admission
// binds them: each entity keeps the union of its last admission, so its first collect takes no region. A static object keeps its
// choice while the snapshot and its box stay; the scales come from the group's own keys' entries (4E2A0 reads them before writing
// the new keys), the cubes from the new keys (75B30: the first missing gives both the neutral cube, the second missing the first's).
struct NativeRegionMemo {uintptr_t model{};bool classes_known{};uint32_t classes{},flags{};uint64_t uid{},serial{},generation{};   // banks, generation: the banks and RecordGeneration an own key's scales were read at
    std::weak_ptr<const ds3light::Banks> banks;
    std::array<float,4> position{};std::array<uint32_t,4> own{};std::shared_ptr<const NativeRegionIbl> ibl;};
NativeEntityShards<NativeRegionMemo> native_region_memos;
// I-0113 tranche 35: the menu views' cubes by key (NativeMenuEnvCube), released at Close with the memos (its fixes' review L3).
struct NativeMenuCubeKept {std::shared_ptr<const bootstrap::NativeTextureRecord> record;uint64_t registry{},names{},looked_ms{};};
std::mutex native_menu_cube_lock;std::map<uint64_t,NativeMenuCubeKept> native_menu_cubes;
void ClearNativeRegionMemos() {native_region_memos.Clear();const std::lock_guard hold(native_menu_cube_lock);native_menu_cubes.clear();}
void NoteNativeRegionClasses(uintptr_t entity,uintptr_t model,uint32_t classes,uint32_t flags,uint64_t collect) {
    auto& shard=native_region_memos.Of(entity);const std::lock_guard hold(shard.lock);auto& memo=shard.map[entity];
    if(memo.model!=model)memo=NativeRegionMemo{model};
    memo.classes_known=true;memo.classes=classes;memo.flags=flags;memo.serial=collect;
}
std::shared_ptr<const NativeRegionIbl> ChooseNativeRegionIbl(const NativeSelectedScene& scene,const NativeSelectedModel& selected,const NativeSelectionCandidate& c) {
    if(!scene.regions || !c.region_box || c.effect || selected.phantom || !selected.ordinary_category ||
       (*selected.ordinary_category!=2 && *selected.ordinary_category!=0x29))return nullptr;
    alignas(16) float box[8]{};std::array<uint32_t,4> own{};
    if(!Read(c.region_box,box,sizeof box) || !Read(selected.group+0xB0,own.data(),16))return nullptr;
    // D3DA20 and D079C0: (minimum + maximum) * 0.5 with .w 1; CFDE70 with the box's own .w.
    const std::array<float,4> position{(box[4]+box[0])*0.5f,(box[5]+box[1])*0.5f,(box[6]+box[2])*0.5f,c.region_box_w?(box[7]+box[3])*0.5f:1.f};
    const auto& regions=*scene.regions;
    auto& shard=native_region_memos.Of(c.entity);const std::lock_guard hold(shard.lock);
    const auto found=shard.map.find(c.entity);
    if(found==shard.map.end() || found->second.model!=selected.model || !found->second.classes_known)return nullptr;
    auto& memo=found->second;memo.serial=scene.serial;
    const uint32_t term=*selected.ordinary_category==0x29?0xFFFFu:0x12u|((memo.flags>>12&1)?0x80u:0u);
    if(!(memo.classes&term))return nullptr;
    // I-0141 tranche 23's final review M1: an own key other than {0, -1} reads its scale from the banks (NativeEnvMapScale), which a
    // fade of its area moves at each step, and a new publication replaces; the region choice does not depend on them. So a memo whose
    // regions, position and keys hold keeps its choice and renews its scales alone when the collect's banks (regions.banks, compared
    // as owners) or their RecordGeneration moved ({0, -1} is regions.default_scale, in regions.uid).
    const bool own_scales=!(own[0]==0 && own[1]==0xFFFFFFFFu && own[2]==0 && own[3]==0xFFFFFFFFu);
    const auto scale_of=[&](size_t k){return own[2*k]==0 && own[2*k+1]==0xFFFFFFFFu?regions.default_scale:NativeEnvMapScale(own.data()+2*k);};
    const bool same_banks=!memo.banks.owner_before(regions.banks) && !regions.banks.owner_before(memo.banks);
    if(memo.ibl && memo.uid==regions.uid && memo.position==position && memo.own==own){
        if(!own_scales || (same_banks && memo.generation==regions.record_generation))return memo.ibl;
        const float scales[2]{scale_of(0),scale_of(1)};memo.banks=regions.banks;memo.generation=regions.record_generation;
        if(scales[0]!=memo.ibl->scales[0] || scales[1]!=memo.ibl->scales[1]){auto renewed=std::make_shared<NativeRegionIbl>(*memo.ibl);
            renewed->scales[0]=scales[0];renewed->scales[1]=scales[1];memo.ibl=std::move(renewed);}
        return memo.ibl;}
    // The main view's masks +0x4B8/+0x4BC are {1, 1} (OBSERVED on every pass-0x45 row, i113-t14-al-original).
    static constexpr uint32_t view[2]{1,1};
    const auto choice=ds3native::env_region::Choose(regions.listed,regions.fallback,position.data(),view);
    // 75B30: a key whose id is -1 binds the REM (the device's +0x178), absent natively; a cube the engine has and the native not
    // yet resident is bound as it is, so the row waits for it (D-0197), not drawn black.
    const auto cube=[&](const ds3native::env_region::Region* r)->std::shared_ptr<const bootstrap::NativeTextureRecord>{
        if(r->key[1]==0xFFFFFFFFu)return nullptr;
        return (r==&regions.fallback?regions.fallback_cube:regions.cubes[static_cast<size_t>(r-regions.listed.data())]).record;};
    auto ibl=std::make_shared<NativeRegionIbl>();
    std::memcpy(ibl->keys,choice.first->key,8);std::memcpy(ibl->keys+2,choice.second->key,8);ibl->blend=choice.blend;
    for(size_t k=0;k<2;++k)ibl->scales[k]=scale_of(k);
    ibl->bound[0]=cube(choice.first);
    if(ibl->bound[0]){ibl->bound[1]=cube(choice.second);if(!ibl->bound[1])ibl->bound[1]=ibl->bound[0];}
    // Witness (light-observe.flag): once per model name, keys and blend in hundredths (at most 600 rows), as reference_region_ibl.
    static const bool observe=ModuleFlag(L"light-observe.flag");
    static std::atomic<bool> witness_full{};
    if(observe && !witness_full.load(std::memory_order_relaxed)){static std::mutex lock;static std::set<std::tuple<std::string,uint32_t,uint32_t,int>> seen;
        const auto name=NativeModelDataName(selected.model);bool fresh{};
        {const std::lock_guard held(lock);fresh=seen.insert({name,ibl->keys[1],ibl->keys[3],static_cast<int>(std::lround(ibl->blend*100.f))}).second;if(seen.size()>=600)witness_full=true;}
        if(fresh){std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[400]{};
            sprintf_s(row,"{\"event\":\"native_region_ibl\",\"model\":\"%.60s\",\"keys\":[[%u,%d],[%u,%d]],\"blend\":%g,\"scales\":[%g,%g],\"position\":[%g,%g,%g,%g],\"cubes\":[%s,%s]}\n",
                clean.c_str(),ibl->keys[0],static_cast<int32_t>(ibl->keys[1]),ibl->keys[2],static_cast<int32_t>(ibl->keys[3]),ibl->blend,ibl->scales[0],ibl->scales[1],
                position[0],position[1],position[2],position[3],ibl->bound[0]?"true":"false",ibl->bound[1]?"true":"false");Log(row);}}
    // The same choice keeps the memo's pointer (a kept draw's input: an unrelated cube becoming ready changes the uid, not this).
    const auto same=[](const NativeRegionIbl& a,const NativeRegionIbl& b){return !std::memcmp(a.keys,b.keys,sizeof a.keys) && a.blend==b.blend &&
        a.scales[0]==b.scales[0] && a.scales[1]==b.scales[1] && a.bound[0]==b.bound[0] && a.bound[1]==b.bound[1];};
    memo.uid=regions.uid;memo.position=position;memo.own=own;memo.banks=regions.banks;memo.generation=regions.record_generation;if(!memo.ibl || !same(*memo.ibl,*ibl))memo.ibl=std::move(ibl);
    return memo.ibl;
}
// I-0113 tranche 35 (the Face Detail's and the portraits' darker hair): natively t30/t31 were the black cube for every menu view
// (13b took the region IBL for the main view only). In the original (OBSERVED at the Face Detail, i66-reference-i113-t35-face) the
// menu view's forward groups own the keys {0, -1} and 5C0E0's 4E2A0 writes them the region keys {1, 10} where the class term meets
// the model (`reference_forward_row_position`: the term 0x92 rows written, the 0x12 and 0 rows not, as tranche 14's main-view rule),
// blend 0; 75B30 then binds the {1, 10} entry's cube, which 3FEB0 names « GILM » + %04d (area 1's map name empty) and 14006CED0 asks
// [[GXRS+42A8]+250] for: 140D49B10, the CPU texture registry [144792F58] by name, the lookup D24490 makes (READ; a 128x128
// R16G16B16A16_FLOAT cube, 8 mips). Natively here: the same term rule on the entity's memo (NoteNativeRegionClasses, now for the
// menu views too), the key {1, 10} (below: the menu views' region choice is not transcribed), the cube by the original's name
// (native_cube_names, then the registry), the scales 4E2A0 takes from the group's own keys' entries (NativeEnvMapScale, as
// ChooseNativeRegionIbl), blend 0. None (the black cube, as before) while no resident cube is found under that name.
std::shared_ptr<const bootstrap::NativeTextureRecord> NativeMenuEnvCube(const uint32_t key[2],std::string& name,std::string& state) {
    char text[32]{};sprintf_s(text,"GILM%04d",static_cast<int32_t>(key[1]));name=text;   // area 1's entries (its map name empty)
    const auto registry=texture_registry_generation.load(std::memory_order_acquire);
    const auto names=native_cube_names_generation.load(std::memory_order_acquire);const auto now=GetTickCount64();
    const std::lock_guard hold(native_menu_cube_lock);
    auto& k=native_menu_cubes[uint64_t(key[0])<<32|key[1]];
    const bool live=k.record && !k.record->withdrawn.load(std::memory_order_acquire);
    if(live && k.registry==registry && k.names==names){state="bound";return k.record;}
    if(!live && k.registry==registry && k.names==names && now<k.looked_ms+500){state="absent";return nullptr;}
    // The step review's M1: a lookup a registry writer made busy keeps the previous cube and tries again next time.
    uintptr_t receiver{};{const std::lock_guard names_hold(native_cube_name_lock);const auto found=native_cube_names.find(name);if(found!=native_cube_names.end())receiver=found->second;}
    std::shared_ptr<const bootstrap::NativeTextureRecord> record;
    if(receiver)if(auto r=textures.Acquire(receiver);r && r->resident && r->resource && r->view)record=std::move(r);
    if(!record){const std::wstring wide(name.begin(),name.end());bool busy{};const auto holder=NativeTextureRegistryFind(0,wide.c_str(),busy);
        if(busy){state="busy";k.registry=0;return live?k.record:nullptr;}
        uintptr_t table{};if(holder && Read(holder,&table,8) && table==image+0x29559E8){std::array<uintptr_t,4> holders{holder,0,0,0},inners{};
            record=ResolveNativeTextureHolders(holders,inners);}}
    k.registry=registry;k.names=names;k.looked_ms=now;
    if(record && !record->description.cube){state="not a cube";record.reset();}
    else state=record?"bound":"absent";
    k.record=record;return record;
}
// I-0140 tranche 30 (the Load Game list's third row warmer and darker natively, i140-t30-*-nat): each menu view's admissions by
// outcome (category, memo, term, no cube, bound), counted per collect for the view being admitted (native_offscreen_view_ibl rows).
thread_local std::array<uint32_t,5> native_menu_ibl_tally{};
std::shared_ptr<const NativeRegionIbl> NativeMenuRegionIbl(const NativeSelectedModel& selected,const NativeSelectionCandidate& c) {
    // A row for each reason a menu model takes no cube, its first 4 times (the witness of the gates below).
    const auto none=[](uint32_t reason)->std::shared_ptr<const NativeRegionIbl>{static std::array<std::atomic<int>,3> told{};
        if(reason<3)++native_menu_ibl_tally[reason];
        if(reason<told.size() && told[reason].fetch_add(1)<4){static constexpr const char* names[3]{"category","memo","term"};char row[128]{};
            sprintf_s(row,"{\"event\":\"native_menu_ibl_none\",\"reason\":\"%s\"}\n",names[reason]);Log(row);}
        return nullptr;};
    if(c.effect || selected.phantom || !selected.ordinary_category || (*selected.ordinary_category!=2 && *selected.ordinary_category!=0x29))return none(0);
    uint32_t classes{},flags{};
    {auto& shard=native_region_memos.Of(c.entity);const std::lock_guard hold(shard.lock);const auto found=shard.map.find(c.entity);
        if(found==shard.map.end() || found->second.model!=selected.model || !found->second.classes_known)return none(1);
        classes=found->second.classes;flags=found->second.flags;}
    const uint32_t term=*selected.ordinary_category==0x29?0xFFFFu:0x12u|((flags>>12&1)?0x80u:0u);
    if(!(classes&term))return none(2);
    // The key: {1, 10}, the area-1 entry the original's menu rows took at the creation (OBSERVED there only). Its fixes' review H1:
    // the region manager's default handle resolves to {0, -1} in game (every native_region_snapshot row) and the manager was off
    // natively at the creation before I-0140 tranche 28 (`i66-native-i113-t35c-face`), so 4E2A0's own choice for a menu view (its masks {1, 2}, the model's
    // position near the origin) is not transcribed (I-0113 RESULT « Tranche 35 », Not done here); the default handle's key is logged.
    // Since I-0140 tranche 28 the manager holds the menu region natively (D0C2F0: key {1, 10}, masks {0x11, 2}), the key this
    // constant names; reading 4E2A0's choice from it instead is left open (I-0140 RESULT « Tranche 28 »).
    static constexpr uint32_t key[2]{1,10};std::array<uint32_t,4> own{};
    if(!Read(selected.group+0xB0,own.data(),16))own={0,0xFFFFFFFFu,0,0xFFFFFFFFu};   // a fresh group's own keys
    std::string name,state;auto cube=NativeMenuEnvCube(key,name,state);
    float scales[2]{};if(cube)for(size_t k=0;k<2;++k)scales[k]=NativeEnvMapScale(own.data()+2*k);
    {static std::mutex rows_lock;static std::set<std::string> seen;bool fresh{};
        {const std::lock_guard hold(rows_lock);fresh=seen.size()<24 && seen.insert(state).second;}
        if(fresh){uint32_t default_key[2]{0,0xFFFFFFFFu};uintptr_t root{};std::array<uint8_t,0x68> m{};   // the final review's L2: read for the row only
            if(Read(image+0x4961B50,&root,8) && root && Read(root+0x42B0,m.data(),m.size()) && m[0x10]){
                const uintptr_t manager=root+0x42B0;uintptr_t entries{};std::memcpy(&entries,m.data()+0x58,8);uint32_t count{};std::memcpy(&count,m.data()+0x60,4);
                uintptr_t region=manager+0x80;int32_t id{};uint16_t generation{},current{};uintptr_t r{};const uintptr_t handle=manager+0x70;
                if(Read(handle+8,&id,4) && id!=-1){const uint16_t index=static_cast<uint16_t>(id);
                    if(count>=index && Read(handle+0xA,&generation,2) && Read(entries+0x10+0x18ull*index,&current,2) && generation==current && Read(entries+0x18ull*index,&r,8) && r)region=r;}
                if(!Read(region,default_key,8)){default_key[0]=0;default_key[1]=0xFFFFFFFFu;}}
            char row[360]{};sprintf_s(row,"{\"event\":\"native_menu_ibl\",\"key\":[%u,%d],\"default_key\":[%u,%d],\"manager\":%d,\"name\":\"%.40s\",\"state\":\"%s\",\"width\":%u,\"mips\":%u,\"format\":%u,\"own\":[[%u,%d],[%u,%d]],\"scales\":[%g,%g]}\n",
                key[0],static_cast<int32_t>(key[1]),default_key[0],static_cast<int32_t>(default_key[1]),int(m[0x10]),name.c_str(),state.c_str(),cube?cube->description.width:0u,cube?cube->description.mips:0u,
                cube?unsigned(cube->description.format):0u,own[0],static_cast<int32_t>(own[1]),own[2],static_cast<int32_t>(own[3]),scales[0],scales[1]);Log(row);}}
    if(!cube){++native_menu_ibl_tally[3];return nullptr;}
    ++native_menu_ibl_tally[4];
    auto ibl=std::make_shared<NativeRegionIbl>();ibl->keys[0]=ibl->keys[2]=key[0];ibl->keys[1]=ibl->keys[3]=key[1];ibl->blend=0.f;
    ibl->scales[0]=scales[0];ibl->scales[1]=scales[1];
    ibl->bound[0]=ibl->bound[1]=std::move(cube);
    return ibl;
}
// The material's FC_BlendMaskParam (cbMatDynParam +0x30) as 22E930's fixed block makes it: with its dynamic object's (+0xD0) +0C or
// +19 flag, {a, b, -0.5 a, 0.5 - 0.5 b} from the object's +28 and +2C (not checked finite: the block's writer refuses a value that
// is not); none when both flags are clear or a read fails (the block not written). The admission's block 0x30 and the rays' MulMask
// record (I-0119 R3c, 12_scene_collect.inc) both take it from here.
std::optional<std::array<float,4>> NativeBlendMaskParam(uintptr_t material) noexcept {
    std::array<BYTE,0x28> flags{};std::array<float,2> v{};
    if(!material || !Read(material+0xD0,flags.data(),flags.size()) || !(flags[0x0C] || flags[0x19]) || !Read(material+0xD0+0x28,v.data(),8))return std::nullopt;
    return std::array<float,4>{v[0],v[1],v[0]*-0.5f,0.5f*(1.f-v[1])};
}
// 1096 (22E930 asm 14022FF53-14023002A): the material's FC_DynamicDiffuseColor (cbMatDynParam +0x180) from its dynamic object
// `source` (material +0xD0) and that object's first 0x28 bytes `flags`, set when the flag +0x20 is. Provider p=[+360] absent: the
// MTD's g_DiffuseColor provider (`material`'s: natively no parameter object exists) or the 16 bytes at 4929940. Else colour =
// [+350..+35C] when p+54 and object+1E are set, otherwise *p->vt+70(p); scale = [+35C] when p+41 and object+1F are set, otherwise
// [p+50]; result {s*r,s*g,s*b,[492994C]}. False (and zero): the flag clear or a read failed (the block not written). The
// admission's block 0x180 and the rays' forward records (I-0125 R6d, 12_scene_collect.inc) both take it from here.
bool NativeDynamicDiffuseColour(uintptr_t source,const std::array<BYTE,0x28>& flags,const NativeModelMaterial* material,std::array<float,4>& colour) {
    colour={};bool set=flags[0x20]!=0;
    if(set){uintptr_t provider{};
        const auto* mtd_provider=material && material->diffuse_provider?&*material->diffuse_provider:nullptr;
        if(!Read(source+0x360,&provider,8))set=false;
        else if(!provider && mtd_provider){   // natively the MTD's g_DiffuseColor provider (see NativeModelMaterial)
            float alpha{},scale{};set=Read(image+0x492994C,&alpha,4);
            if(set && mtd_provider->use_colour && flags[0x1E])set=Read(source+0x350,colour.data(),16);
            else colour={mtd_provider->colour[0],mtd_provider->colour[1],mtd_provider->colour[2],0.0f};
            if(set)set=mtd_provider->use_power && flags[0x1F]?Read(source+0x35C,&scale,4):(scale=mtd_provider->power,true);
            if(set)colour={scale*colour[0],scale*colour[1],scale*colour[2],alpha};}
        else if(!provider)set=Read(image+0x4929940,colour.data(),16);
        else{BYTE own_colour{},own_scale{};float scale{},alpha{};
            set=Read(provider+0x54,&own_colour,1) && Read(provider+0x41,&own_scale,1) && Read(image+0x492994C,&alpha,4);
            if(set && own_colour && flags[0x1E])set=Read(source+0x350,colour.data(),16);
            else if(set){uintptr_t table{},query{};
                set=Read(provider,&table,8) && Read(table+0x70,&query,8) && query>=image && query<image+0x2800000;
                if(set){const auto values=reinterpret_cast<const float*(__fastcall*)(uintptr_t)>(query)(provider);
                    set=values && Read(reinterpret_cast<uintptr_t>(values),colour.data(),16);}}
            if(set)set=own_scale && flags[0x1F]?Read(source+0x35C,&scale,4):Read(provider+0x50,&scale,4);
            if(set)colour={scale*colour[0],scale*colour[1],scale*colour[2],alpha};}}
    if(!set)colour={};
    return set;
}
// 1185: a cloth instance frame (I+0x50 current, I+0x90 previous: 4x4 rows, translation last) as VC_World's 3x4 rows (its
// transpose), the frame the _cloth programmes place a simulated copy by; the raster's cloth draws (cloth_world) and, since I-0113
// tranche 37 (gap 25), the rays' mode-3 cloth slices take it. False when unreadable; `finite` (when given) whether every word is
// finite (the raster copies the rows as they are, as the original does, its step review's M1; the rays leave the cloth slices out of that walk).
bool NativeClothWorldRows(uintptr_t cloth_instance,uint32_t offset,std::array<float,12>& out,bool* finite=nullptr) noexcept {
    std::array<float,16> m{};if(!cloth_instance || !Read(cloth_instance+offset,m.data(),64))return false;
    for(int r=0;r<3;++r)for(int k=0;k<4;++k)out[r*4+k]=m[k*4+r];
    if(finite)*finite=std::all_of(out.begin(),out.end(),[](float v){return std::isfinite(v);});
    return true;}
void AdmitNativeModelCandidate(NativeSelectionVisitor* visitor,const NativeSelectionCandidate& c) {
    const kprofile::Scope k_model(kprofile::ModelAdmission);
    native_admission_code=0;
    AdmitNativeModelCandidateBody(visitor,c);
}
void AdmitNativeModelCandidateBody(NativeSelectionVisitor* visitor,const NativeSelectionCandidate& c) {
    auto& result=*visitor->result;
    // I-0101 (« skip the object and go on »): a non-finite placement skips this candidate (the original draws it as it is, its
    // triangles dropped by the rasterizer), where it used to close the game.
    for(const auto value:c.placement)if(!std::isfinite(value)){native_admission_code=7,++result.nonfinite;NoteNativeSkip("nonfinite_placement",c.model);return;}
    try {
        const auto model=c.model;
        // I-0078: the draw group (the engine bytes' 0x190, which the fade, scroll, wet, category, capability, instance and light
        // key reads all fall in), read once for this admission. (Not the model object: its inputs are read twice on purpose, to
        // see a change while they are read.)
        const NativeReadWindow group_window(c.group,0x190);
        // 1183 witness (MENUS-MAP 55): the admission outcome of each skinned SprjModelDrawEntity (a character: enemy, NPC),
        // once per entity, with its model data name (+2B8) and bone count (pose prefix +70).
        const char* character_outcome="admitted";
        struct CharacterWitness{const NativeSelectionCandidate& c;const char*& outcome;~CharacterWitness(){
            static const bool witness=ModuleFlag(L"light-observe.flag");if(!witness)return;   // optimisation review: a diagnostic (Native2219: 1 % of the engine thread)
            if(c.neutral_instance_decal)return;   // 1183: characters and map/object items (o, c models), not menu or player parts
            uint32_t bones{};if(c.pose)Read(c.pose+0x70,&bones,4);
            {uint64_t capacity{};uintptr_t text=c.model+0x2B8;wchar_t first[2]{};Read(c.model+0x2D0,&capacity,8);if(capacity>7)Read(c.model+0x2B8,&text,8);
                if(!Read(text,first,4) || (first[0]!=L'c' && first[0]!=L'C' && first[0]!=L'o' && first[0]!=L'O') || first[1]<L'0' || first[1]>L'9')return;}
            static std::mutex lock;static std::map<uintptr_t,std::string> seen;bool fresh{};
            {const std::lock_guard hold(lock);const auto found=seen.find(c.entity);if(found==seen.end() && seen.size()>=1500)return;auto& last=seen[c.entity];fresh=last!=outcome;last=outcome;}
            if(!fresh)return;std::string name;{uint64_t capacity{};uintptr_t text=c.model+0x2B8;Read(c.model+0x2D0,&capacity,8);if(capacity>7)Read(c.model+0x2B8,&text,8);
                for(int i=0;i<40;++i){wchar_t ch{};if(!Read(text+2ull*i,&ch,2) || !ch)break;name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');}}
            char row[400]{};sprintf_s(row,"{\"event\":\"native_character_candidate\",\"entity\":%llu,\"model\":%llu,\"name\":\"%s\",\"bones\":%u,\"outcome\":\"%s\"}\n",
                static_cast<unsigned long long>(c.entity),static_cast<unsigned long long>(c.model),name.c_str(),bones,outcome);Log(row);}} character_witness{c,character_outcome};
        // I-0091: on an admission worker the model is built in its candidate's slot, which the merge moves once into its place
        // in the scene; a refused or failed admission leaves the slot empty.
        std::optional<NativeSelectedModel> local;
        auto& slot=native_admission_sink?native_admission_sink[native_admission_index]:local;
        struct SlotGuard {std::optional<NativeSelectedModel>& slot;bool kept{};~SlotGuard(){if(!kept)slot.reset();}} slot_guard{slot};
        NativeSelectedModel& selected=slot.emplace();
        selected.entity=c.entity;selected.owner=c.fade_owner;selected.model=model;selected.group=c.group;selected.animation=c.animation;selected.effect=c.effect;
        admission_marks.Start();
        if(c.effect)selected.fade=NativeEffectModelFade(*c.effect);
        else if(!ingress::ReadModelFadeStateAt(c.fade_owner,c.fade_layout,selected.fade)){   // I-0101: skipped, not refused
            character_outcome="fade_unreadable";native_admission_code=2,++result.missing_bindings;NoteNativeSkip("entity_fade_unreadable",c.model);return;}
        // Optimisation review (the m33 crosses o331204-o331208 and the hollows c1370, zero_fade natively, drawn by the original):
        // the original's emits (D06E10 for draw entities, D04B90 for map objects) take the geometry weight (+0x170 / +0x130,
        // the LodParam's second distance) only for views of engine passes 1..0x38, and the entity's track weight for the main
        // view, whose type is 0x45 (FUN_1400575C0; MENUS-MAP 62 sections 3 and 5). The main view gathers the category-0x01
        // entities (E+0x20 bit 0x01, C2EC0); the category-0x02/0x04 ones only come through the pass 5..8 / 9..0x38 views. So the
        // main walk draws an entity when either view would: the pass-5 view for categories 0x02/0x04 (the geometry weight, and
        // the suppress bytes E+0xA5D / item+0x71A, which D06E10 / D3DA20 test only for passes 5..8: the probe pick 1D650 sets
        // them for the entities it lights, Native2258 at m40: the player and the candles), else the 0x45 view for category 0x01
        // (the track weight, no suppress test). The shadow and menu walks keep the pass-5 rule.
        uint32_t engine_pass=5;
        if(c.geometry_suppressed)selected.fade.geometry_suppressed=true;
        if(!ingress::SelectModelFade(selected.fade,5,selected.geometry_fades)){
            character_outcome="fade_unreadable";native_admission_code=2,++result.missing_bindings;NoteNativeSkip("entity_fade_unreadable",c.model);return;}
        if(!native_offscreen_preparing && !c.effect){uint8_t category{};
            // I-0115 (Thomas, 2026-11-02: the flesh masses and the Silver Knights gone and back within half a second): an entity of
            // categories 0x01 and 0x02/0x04 is gathered by both views. While its geometry weight is 0 (beyond the LodParam's second
            // distance, c2131's 15 / 0.757 = 19.8 m, c1410's 27.7 m) the 0x45 view drew it at its track weight 1; at that distance
            // the geometry weight starts its ramp (0.083 a step) and the pass-5 view took over: the character fell to 8 % and came
            // back over 11 frames (popin_native_fade, i115-hall-n60). So the more opaque of the two views draws it (INFERRED: what
            // the screen shows of the original's two views' draws, popin_reference_fade's o5 / o45); no-view-weight-fix.flag gives
            // the former rule (the pass-5 view whenever it has an intent). Accepted: such a model's shadow rows then take the 0x45
            // intent too, so its shadow starts at full strength instead of the geometry weight's 11-frame ramp.
            static const bool view_weight_fix=!ModuleFlag(L"no-view-weight-fix.flag");
            if(Read(c.fade_owner+0x20,&category,1) && (category&1u) && (!(category&6u) || !selected.geometry_fades.count || view_weight_fix)){
                const bool geometry_view=(category&6u) && selected.geometry_fades.count;
                ingress::ModelFadeSelection main_view;
                if(!ingress::SelectModelFade(selected.fade,0x45,main_view)){
                    // Step review L1: an unreadable track weight keeps the pass-5 selection the former rule drew.
                    if(!geometry_view){character_outcome="fade_unreadable";native_admission_code=2,++result.missing_bindings;NoteNativeSkip("entity_fade_unreadable",c.model);return;}
                    main_view={};}
                if(ingress::PreferMainViewFade(category,selected.geometry_fades,main_view,view_weight_fix)){
                    selected.geometry_fades=main_view;engine_pass=0x45;}}}   // category 0x01 alone: only the 0x45 view
        // I-0140 tranche 5 witness (character-fade-observe.flag, its own: its lock and name read on every main-walk admission; 31000010's
        // wyverns, c2021, almost invisible natively): for the character
        // models (data name « c… ») of the main walks, the admissions counted by the opacity of their first intent (none, below 1,
        // full), one row each 600 admissions of a name (24 names, 400 rows at most), with the last fade words and view.
        static const bool fade_witness=ModuleFlag(L"character-fade-observe.flag");
        if(fade_witness && !native_offscreen_preparing && !native_rt_gates && !c.effect){const auto name=NativeModelDataName(model);
            if(name.size()>1 && (name[0]=='c' || name[0]=='C') && name[1]>='0' && name[1]<='9'){
                struct Tally{uint64_t none{},partial{},full{};};static std::mutex lock;static std::map<std::string,Tally> tallies;static uint32_t rows{};
                const float opacity=selected.geometry_fades.count?selected.geometry_fades.intents[0].opacity:0.f;bool emit{};Tally now{};
                {const std::lock_guard held(lock);auto found=tallies.find(name);
                    if(found==tallies.end() && tallies.size()<24)found=tallies.emplace(name,Tally{}).first;
                    if(found!=tallies.end()){auto& t=found->second;(!selected.geometry_fades.count?t.none:opacity<0.999f?t.partial:t.full)+=1;
                        emit=(t.none+t.partial+t.full)%600==1 && rows<400;if(emit){++rows;now=t;}}}
                if(emit){const auto& f=selected.fade;char row[512]{};sprintf_s(row,"{\"event\":\"native_character_fade\",\"model\":\"%.40s\",\"disp\":%s,\"none\":%llu,\"partial\":%llu,\"full\":%llu,\"view\":%u,\"opacity\":%g,\"track\":%g,\"geometry_weight\":%g,\"suppressed\":%s,\"group_opacity\":%g,\"normal\":[%d,%d,%g]}\n",
                    name.c_str(),c.fade_owner!=c.entity?"true":"false",static_cast<unsigned long long>(now.none),static_cast<unsigned long long>(now.partial),static_cast<unsigned long long>(now.full),
                    engine_pass,opacity,f.normal.weight,f.geometry_weight,f.geometry_suppressed?"true":"false",f.group_opacity,f.normal.current,f.normal.previous,f.normal.interpolation);Log(row);}}}
        // Witness: the models the main view's weight admits where the geometry pass's would not (once per name, 96 names).
        static const bool weight_witness=ModuleFlag(L"light-observe.flag");
        if(weight_witness && engine_pass==0x45 && selected.geometry_fades.count){ingress::ModelFadeSelection former;
            if(ingress::SelectModelFade(selected.fade,5,former) && !former.count){static std::mutex lock;static std::set<std::string> seen;const auto name=NativeModelDataName(model);bool fresh{};
                {const std::lock_guard held(lock);fresh=seen.size()<96 && seen.insert(name).second;}
                if(fresh){char row[320]{};sprintf_s(row,"{\"event\":\"native_main_view_weight_admitted\",\"model\":\"%.60s\",\"track_weight\":%g,\"geometry_weight\":%g,\"placement\":[%.1f,%.1f,%.1f]}\n",
                    name.c_str(),selected.fade.normal.weight,selected.fade.geometry_weight,c.placement[3],c.placement[7],c.placement[11]);Log(row);}}}
        // Geometry candidates use the actual 5..8 fade family, not pass0's
        // menu weight. Other pass intents remain derivable from owned inputs.
        if(!selected.geometry_fades.count){character_outcome="zero_fade";native_admission_code=1,++result.zero_fade;
            // Fable review (m33: c1370, c1441, c2271... refused as zero_fade while the original draws them): the fade words
            // of the first refusals, once per model name.
            {static std::mutex lock;static std::set<std::string> seen;const auto name=NativeModelDataName(model);bool fresh{};
                {const std::lock_guard held(lock);fresh=seen.size()<48 && seen.insert(name).second;}
                if(fresh){const auto& f=selected.fade;char row[640]{};sprintf_s(row,"{\"event\":\"native_zero_fade\",\"model\":\"%.60s\",\"asm\":%s,\"owner_is_entity\":%s,\"normal\":[%d,%d,%g,%g,%d,%d],\"alternate\":[%d,%d,%g,%g,%d,%d],\"geometry_weight\":%g,\"suppressed\":%s,\"group_opacity\":%g,\"group_flags\":%u,\"fade_enabled\":%s,\"placement\":[%.1f,%.1f,%.1f]}\n",
                    name.c_str(),c.capability_source?"true":"false",c.fade_owner==c.entity?"true":"false",f.normal.current,f.normal.previous,f.normal.interpolation,f.normal.weight,f.normal.current_variant,f.normal.previous_variant,
                    f.alternate.current,f.alternate.previous,f.alternate.interpolation,f.alternate.weight,f.alternate.current_variant,f.alternate.previous_variant,f.geometry_weight,f.geometry_suppressed?"true":"false",f.group_opacity,f.group_flags,f.fade_enabled?"true":"false",
                    c.placement[3],c.placement[7],c.placement[11]);Log(row);}}
            return;}
        ds3content::ModelParameterAnimations parameter_animations;
        // Genuine current CPU collection at entity+A40, independently routed
        // from the colour/light animation object at entity+3D0. No old draw
        // context is constructed; failure leaves numeric writes unavailable.
        if(c.parameter_animations)ingress::ReadModelParameterAnimations(c.parameter_animations,parameter_animations);
        // An owner without a collection (null descriptor+20) has no animation rows: generated values stand.
        else if(c.fade_owner!=c.entity)parameter_animations.available=true;
        {uint32_t nonzero{};for(uint32_t i=0;i<selected.geometry_fades.count;++i)nonzero+=selected.geometry_fades.intents[i].variant!=0;
            if(nonzero)result.nonzero_variant+=nonzero;}   // I-0078: one atomic add a model at most
        admission_marks.Mark(0);   // m45 profile: fade state and selection, parameter animations
        ingress::CurrentModelInputs inputs;
        if(!ingress::ReadCurrentModelInputsAt(image,model,c.source_field,c.instance_field,c.pose,inputs)){
            // I-0100 tranche 2 (R9: o000499, o319250, m31_00_00_00_009400 never drawn natively, refused here): which check of the
            // current inputs fails, by model name at its 1st, 100th, 1,000th and 10,000th refusal (popin-observe.flag, 64 names: does a
            // model stay unloaded?): the membership words, the instance vector and the first mesh whose material is missing.
            uint64_t refusals{};
            if(PopinObserve()){static std::mutex lock;static std::unordered_map<std::string,uint64_t> seen;const auto name=NativeModelDataName(model);bool fresh{};
                {const std::lock_guard held(lock);
                    if(seen.contains(name) || seen.size()<64){refusals=++seen[name];fresh=refusals==1 || refusals==100 || refusals==1000 || refusals==10000;}}
                if(fresh){uintptr_t table{},storage{},meshes{},begin{},end{},source{},vector{},first{},last{};int32_t count{};
                    Read(model,&table,8);Read(model+0xE8,&storage,8);Read(model+0xA0,&count,4);Read(model+0xA8,&meshes,8);
                    Read(model+0x1F8,&begin,8);Read(model+0x200,&end,8);if(c.source_field)Read(c.source_field,&source,8);
                    if(c.instance_field && Read(c.instance_field,&vector,8) && vector){Read(vector,&first,8);Read(vector+8,&last,8);}
                    const long long material_count=end>=begin?static_cast<long long>((end-begin)/8):-1ll;int bad_mesh=-1;uint32_t bad_id{};
                    for(int32_t k=0;meshes && k<count && k<4096 && bad_mesh<0;++k){uint16_t id{};uintptr_t material{};
                        if(!Read(meshes+0x48ull*k,&id,2) || id>=material_count || !Read(begin+8ull*id,&material,8) || !material){bad_mesh=k;bad_id=id;}}
                    std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');char row[480]{};
                    sprintf_s(row,"{\"event\":\"popin_native_missing_inputs\",\"model\":\"%.60s\",\"table_rva\":%lld,\"storage\":%s,\"count\":%d,\"meshes\":%s,\"materials\":%lld,"
                        "\"source\":%s,\"instance_vector\":%s,\"instance_bytes\":%lld,\"bad_mesh\":%d,\"bad_material_id\":%u,\"refusals\":%llu,\"serial\":%llu}\n",clean.c_str(),table?static_cast<long long>(table-image):-1ll,
                        storage?"true":"false",count,meshes?"true":"false",material_count,source?"true":"false",vector?"true":"false",
                        last>=first?static_cast<long long>(last-first):-1ll,bad_mesh,bad_id,static_cast<unsigned long long>(refusals),static_cast<unsigned long long>(result.serial));Log(row);}}
            character_outcome="missing_inputs";NoteNativeAsmMissingBinding(c,1);native_admission_code=2,++result.missing_bindings;return;}
        admission_marks.Mark(1);   // m45 profile: the current model inputs
        selected.patch=inputs.patch;selected.instance_streams=inputs.instance_streams;
        // 1185: the simulated copies are in the cloth instance frame: I+0x50 (current) and I+0x90, 4x4 rows with the
        // translation last (measured: rotated 90 degrees from the part placement, same origin); VC_World takes its 3x4 transpose.
        if(c.cloth_set && c.cloth_instance){std::array<float,12> current{},previous{};
            if(NativeClothWorldRows(c.cloth_instance,0x50,current) && NativeClothWorldRows(c.cloth_instance,0x90,previous)){std::array<float,24> rows{};
                std::copy(current.begin(),current.end(),rows.begin());std::copy(previous.begin(),previous.end(),rows.begin()+12);selected.cloth_world=rows;}}
        if(inputs.instance_streams){character_outcome="instance_streams";native_admission_code=3,++result.instance_streams;return;}
        ingress::ModelDecalChoice decal_choice;
        if(ingress::ReadModelDecalChoice(image,model,inputs.patch,decal_choice))selected.decal_choice=decal_choice;
        selected.neutral_instance_decal=c.neutral_instance_decal;
        // DECALS-WAVE1 D1: the model name's first letter (c for characters), read once per selected model.
        {uint64_t capacity{};uintptr_t text=model+0x2B8;wchar_t first{};if(Read(model+0x2D0,&capacity,8) && capacity>7)Read(model+0x2B8,&text,8);
            selected.character=c.neutral_instance_decal || (Read(text,&first,2) && first==L'c');}
        {static const bool positions=ModuleFlag(L"chr-positions.flag");
            if(positions && selected.character){bool fresh{};{const std::lock_guard hold(chair_hunt::seen_lock);fresh=chair_hunt::seen.size()<128 && chair_hunt::seen.insert(c.entity).second;}
                if(fresh){char row[240]{};sprintf_s(row,"{\"event\":\"native_chr_position\",\"name\":\"%.40s\",\"entity\":%llu,\"at\":[%.3f,%.3f,%.3f]}\n",
                    NativeModelDataName(model).c_str(),static_cast<unsigned long long>(c.entity),c.placement[3],c.placement[7],c.placement[11]);Log(row);}}}
        if(c.draw_colour){std::array<uint8_t,16> colour{};if(!Read(c.draw_colour,colour.data(),colour.size())){
            character_outcome="draw_colour_unreadable";native_admission_code=2,++result.missing_bindings;NoteNativeSkip("draw_colour_unreadable",c.model);return;}selected.draw_colour=colour;}
        //818:1A8580 copies the selected group's first64 bytes to state+240;
        //1A86C0/ProduceTexScroll delivers those exact rows at VS b11. Fade
        //copies preserve these words. Own them directly, without a draw state.
        std::array<uint8_t,64> texture_scroll;
        if(Read(c.group,texture_scroll.data(),texture_scroll.size()))selected.texture_scroll=texture_scroll;
        ingress::ModelWetDrawInputs wet_draw_inputs;
        if(ingress::ReadModelWetDrawInputsAt(c.group,wet_draw_inputs))selected.wet_draw_inputs=wet_draw_inputs;
        uint32_t material_mask{};
        if(Read(model+0x220,&material_mask,4))selected.material_mask=material_mask;
        // Native ordinary-main intent corresponds to the source3B..40 family;
        // its initial category2 is recategorized from current CPU group bytes.
        BYTE first_category{},second_category{},instance_category{};float category_value{};
        if(result.category_scalar && result.category_override &&
           Read(c.group+0x84,&first_category,1) && Read(c.group+0x85,&second_category,1) &&
           Read(c.group+0x87,&instance_category,1) && Read(c.group+0xDC,&category_value,4)){
            // P4: CFDE70's per-part phantom override of its stack copy (kind 0: 1 below a faded alpha of 1, else 0xFF).
            if(c.phantom){instance_category=1;category_value*=c.phantom_fade;
                first_category=c.phantom_kind?c.phantom_kind:(category_value<1.f?1:0xFF);}
            uint32_t ordinary_category=2;
            if(second_category==0 && static_cast<BYTE>(first_category+1)<2){
                if(instance_category){ordinary_category=0x21;if(category_value<*result.category_scalar)ordinary_category=0x29;}
            }else if((second_category==1 || first_category==1) && category_value==*result.category_scalar){
                //57A20's comma assignment restores initial2 before testing
                //the remaining condition when instance_category is zero.
                ordinary_category=instance_category?0x21u:2u;
            }else ordinary_category=0x29;
            if(*result.category_override)ordinary_category=0x29;
            selected.ordinary_category=ordinary_category;selected.phantom=c.phantom;
            if(c.phantom){selected.phantom_kind=first_category;selected.phantom_fade=c.phantom_fade;}
            // I-0113 tranche 10d witness (light-observe.flag; gap 11, Chameleon alternating, Hidden Body, gap 19): a model's category
            // and the group bytes that choose it, a row each time they change for that entity and model (600 rows at most), to see
            // whether the alternation is the admission's category flipping.
            static const bool category_witness=ModuleFlag(L"light-observe.flag");
            if(category_witness && !c.effect && !native_offscreen_preparing){
                static std::mutex lock;static std::map<std::pair<uintptr_t,uintptr_t>,uint64_t> last;static uint32_t rows{};static std::atomic<bool> full{};
                if(!full.load(std::memory_order_relaxed)){
                const uint64_t word=uint64_t(ordinary_category)|uint64_t(first_category)<<8|uint64_t(second_category)<<16|uint64_t(instance_category)<<24|
                    uint64_t(category_value<*result.category_scalar)<<32|uint64_t(category_value==*result.category_scalar)<<33|uint64_t(c.fade_prepass)<<34;
                bool fresh{};{const std::lock_guard hold(lock);const auto key=std::make_pair(c.entity,model);const auto found=last.find(key);
                    if(rows<600 && (found==last.end()?(ordinary_category!=2 || first_category!=0xFF || second_category):found->second!=word)){fresh=true;++rows;}
                    if(found!=last.end() || fresh)last[key]=word;if(rows>=600)full=true;}
                if(fresh){const auto name=NativeModelDataName(model);std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                    char row[400]{};sprintf_s(row,"{\"event\":\"native_category_change\",\"model\":\"%.60s\",\"entity\":%llu,\"category\":%u,\"g84\":%u,\"g85\":%u,\"g87\":%u,\"gDC\":%g,\"scalar\":%g,\"override\":%u,\"phantom\":%d,\"prepass\":%d,\"serial\":%llu}\n",
                        clean.c_str(),static_cast<unsigned long long>(c.entity),ordinary_category,first_category,second_category,instance_category,category_value,*result.category_scalar,
                        static_cast<unsigned>(*result.category_override),c.phantom?1:0,c.fade_prepass?1:0,static_cast<unsigned long long>(result.serial));Log(row);}}}
        }
        // E03 (I-0105): 63CB0 draws the model particle in the effect's pass with every mesh mask open (636F0's args +0x30..+0x44 all
        // set), shader pass 0 (Fwd): in 0x29 the generic forward row 0x29 alone, class mask 0xFFFF, as category 0x29 gives it; in 0x21
        // (I-0113, the Estus Flask) the row 0x21 alone, as category 0x21 gives it (its class mask 0xFEFF leaves out class 0x100,
        // SAT_Equip, which no model particle seen carries: INFERRED), never 0x22. The effect context's blend and depth replace the
        // row's when the submission carries them (13b); a pass-0x21 submission carried none (i113-m31d: blend 0, depth 0), so the
        // row 0x21's opaque, depth-writing state stands (INFERRED: as the pictures show, the original's states not read).
        if(c.effect)selected.ordinary_category=c.effect->submission.pass==0x21?0x21:0x29;
        // 1066 witness (character body): a part whose storage part+50 has no native pose.
        if(c.neutral_instance_decal){bool known{};{const std::shared_lock hold(model_pose_lock);known=model_poses.contains(inputs.pose);}
            static LONG64 pose_samples{};
            if(!known && InterlockedIncrement64(&pose_samples)<=12){NativeModelPosePrefix prefix{};int32_t bones{};
                Read(inputs.pose,&prefix,sizeof(prefix));Read(model+0x1C,&bones,4);
                uintptr_t first{};Read(inputs.pose,&first,8);
                char row[400]{};sprintf_s(row,"{\"event\":\"native_asm_part_pose_missing\",\"bones\":%d,\"prefix_model_match\":%s,\"prefix_model\":%s,\"prefix_count\":%d,\"ring\":%d,\"index\":%d,\"matrices\":%s,\"gpu\":%s,\"first_rva\":%lld}\n",
                    bones,prefix.model==model?"true":"false",prefix.model?"true":"false",prefix.count,prefix.ring,prefix.index,prefix.matrices[0]?"true":"false",prefix.gpu[0]?"true":"false",
                    static_cast<long long>(first>=image && first<image+0x6000000?first-image:-1));Log(row);}}
        if(c.effect)selected.pose=selected.previous_pose=c.effect->pose;   // E03 (I-0105): the particle's own rows
        else{const std::shared_lock hold(model_pose_lock);   // I-0078: shared (the pose updates take it alone)
            const auto found=model_poses.find(inputs.pose);
            if(found!=model_poses.end()){
                if(found->second.model!=model)Refuse("native_selected_pose_model_mismatch");
                selected.pose=found->second.current;
                selected.previous_pose=found->second.previous?found->second.previous:found->second.current;
                // 1070 witness: a character part whose native pose was never updated.
                static LONG64 stale_samples{};
                if(c.neutral_instance_decal && !found->second.current && InterlockedIncrement64(&stale_samples)%60==1 && stale_samples<=600){char row[320]{};
                    sprintf_s(row,"{\"event\":\"native_asm_part_pose_stale\",\"owner\":%llu,\"part\":%llu,\"init_caller\":%llu,\"count\":%d,\"ring\":%d,\"updates\":%llu}\n",
                        static_cast<unsigned long long>(c.fade_owner),static_cast<unsigned long long>(c.entity),
                        static_cast<unsigned long long>(found->second.init_caller),found->second.count,found->second.ring,
                        static_cast<unsigned long long>(found->second.updates));Log(row);}}}
        // The owner's current placement (D079C0 7E2300 container, or D3DA20 [+6F8]+60).
        selected.placement=c.placement;selected.velocity_rows=c.velocity;selected.fade_prepass=c.fade_prepass;
        // I-0101 (STUDY §1.5 step 3): the object rows (pose row 0, the instance record's rows when model+0x1E0 is set) not
        // finite: the original draws the model as it is (its triangles dropped by the rasterizer); natively it is skipped this frame.
        {bool finite=true;BYTE object_rows{};
            if(selected.pose && !selected.pose->matrices.empty() && Read(model+0x1E0,&object_rows,1) && object_rows)
                for(const auto value:selected.pose->matrices.front())finite=finite && std::isfinite(value);
            if(!finite){character_outcome="nonfinite";native_admission_code=7,++result.nonfinite;NoteNativeSkip("nonfinite_object_rows",model);return;}}
        const auto storage=inputs.storage;
        {const std::shared_lock hold(model_storage_lock);   // I-0078: shared (the storage admissions take it alone)
            const auto found=model_storage.find(storage);
            if(found==model_storage.end() || found->second->meshes.empty()){
                // I-0097: a storage whose geometry job is still queued is not named missing (it is drawn from a later frame).
                if(found!=model_storage.end() && found->second->geometry_pending.load(std::memory_order_acquire)){}
                else if(c.neutral_instance_decal){static LONG64 asm_missing{};if(InterlockedIncrement64(&asm_missing)<=24){
                    const auto refusal=model_geometry_refusal.find(storage);char row[200]{};sprintf_s(row,"{\"event\":\"native_asm_part_geometry_missing\",\"storage\":%s,\"joined_meshes\":%zu,\"refusal_check\":%d}\n",
                        found!=model_storage.end()?"true":"false",found!=model_storage.end()?found->second->meshes.size():0,refusal!=model_geometry_refusal.end()?refusal->second:-1);Log(row);}}
                // 1199: the same for a character model (data name c....), once per storage: c1100 at the High Wall start is
                // drawn by the original and refused here (Native1198).
                // Each storage is named once (at most 4096), so the per-frame cost stays a set lookup (pre-merge review).
                else{static std::mutex named_lock;static std::set<uintptr_t> named;static size_t reported{};std::string name;
                    {const std::lock_guard named_hold(named_lock);if(named.size()<4096 && reported<64 && named.insert(storage).second)name=NativeModelDataName(model);
                        if(name.size()>1 && name[0]=='c' && name[1]>='0' && name[1]<='9')++reported;else name.clear();}
                    if(!name.empty()){
                        const auto refusal=model_geometry_refusal.find(storage);char row[256]{};
                        sprintf_s(row,"{\"event\":\"native_character_geometry_missing\",\"name\":\"%s\",\"storage\":%llu,\"admitted\":%s,\"joined_meshes\":%zu,\"refusal_check\":%d}\n",
                            name.c_str(),static_cast<unsigned long long>(storage),found!=model_storage.end()?"true":"false",found!=model_storage.end()?found->second->meshes.size():0,
                            refusal!=model_geometry_refusal.end()?refusal->second:-1);Log(row);}}
                character_outcome="missing_geometry";native_admission_code=4,++result.missing_geometry;return;}
            selected.geometry=found->second;}
        // I-0101 final review M1: the five storage mismatches below are the registries' invariants (a stale entry after a missed
        // retirement), so they stay refusals (WORKFLOW « Skip the object and go on »).
        if(inputs.meshes.size()!=selected.geometry->meshes.size())Refuse("native_selected_geometry_mesh_membership");
        if(inputs.patch){const std::lock_guard hold(model_stream_lock);
            const auto found=model_streams.find(inputs.patch);
            if(found!=model_streams.end()){
                if(found->second.model!=model || found->second.storage!=selected.geometry)Refuse("native_selected_stream_model_mismatch");
                selected.streams=found->second.current;
            }else{
                // 1090: [source+50] is a polymorphic GXSR material patch (vt+18 applies it per
                // mesh). The physical model stream (D3F7C0, vt+18 D3FA10) overrides vertex slot 0;
                // GXSR::GXBloodyMaterialPatch (vtable 3D41DF0, vt+18 5E8A0) binds only the decal
                // textures t41-t43/t53, which the parts' decal provider policy owns. It supplies
                // no vertex stream, so the authored geometry stands.
                uintptr_t table{};
                if(!Read(inputs.patch,&table,8) || table!=image+0x3D41DF0){
                    character_outcome="unowned_patch";NoteNativeAsmMissingBinding(c,2,inputs.patch);native_admission_code=2,++result.missing_bindings;return;}
            }}
        if(!selected.streams && c.stream_patch){const std::lock_guard hold(model_stream_lock);
            const auto found=model_streams.find(c.stream_patch);
            const bool owned=found!=model_streams.end() && found->second.model==model && found->second.storage==selected.geometry;
            if(owned)selected.streams=found->second.current;
            static LONG64 face_streams{};if(InterlockedIncrement64(&face_streams)<=8){char row[160]{};sprintf_s(row,"{\"event\":\"native_face_stream\",\"owned\":%s,\"current\":%s}\n",
                owned?"true":"false",selected.streams?"true":"false");Log(row);}}
        admission_marks.Mark(2);   // m45 profile: decal, wet, category, pose, storage, streams
        // I-0113 tranche 14: the main collect's (its snapshot); tranche 35: a menu view's (the step review's L3: one or the other; I-0131 R10a: no shadow walk any more).
        selected.region_ibl=native_offscreen_preparing?NativeMenuRegionIbl(selected,c):ChooseNativeRegionIbl(result,selected,c);
        if(c.region_box && c.sort_rule && !c.effect){NativeSelectedModel::SortBox s;s.rule=c.sort_rule;   // I-0113 tranche 15
            if(Read(c.region_box,s.box.data(),32) && (!c.sort_extra || Read(c.sort_extra,&s.extra,4)))selected.sort=s;}
        if(!ProduceNativeSelectedInstances(selected,!c.effect,result.pass_row_scale)){   // review: a model particle adds no set (I-0105: no G-buffer row reads envData)
            character_outcome="instance_unreadable";native_admission_code=2,++result.missing_bindings;NoteNativeSkip("instance_group_unreadable",c.model);return;}
        admission_marks.Mark(3);   // m45 profile: the instances
        // cP6: prepare's engine bytes, read here. m45 frame cost (Native1842: an allocation of 1.4 KB a model each collect): read into
        // the thread's scratch, and the entity's previous copy is shared again when the bytes are the same: I-0078, a main-walk
        // model with a record shares its record's copy (after the record check below, no lookup of its own); the others share the
        // caches' (by thread: the shadow walk and the menu views' collects each keep theirs; entries unseen for 120 collects are
        // dropped).
        thread_local NativeSelectedModel::EngineBytes engine_scratch;
        {auto& engine=engine_scratch;engine=NativeSelectedModel::EngineBytes{};
            engine.group_read=Read(c.group,engine.group.data(),engine.group.size());
            if(c.animation){engine.block_flag_read=Read(c.animation+0x620,&engine.block_flag,1);engine.matrix_flag_read=Read(c.animation+0x630,&engine.matrix_flag,1);
                if(engine.block_flag_read && engine.block_flag)engine.block_read=Read(c.animation+0x210,engine.animation_block.data(),engine.animation_block.size());}}
        const auto share_cached_engine_bytes=[&]{
            struct EngineCache {std::unordered_map<uintptr_t,std::pair<std::shared_ptr<const NativeSelectedModel::EngineBytes>,uint64_t>> entries;uint64_t sweep{};};
            thread_local EngineCache engine_cache;
            // I-0071 point 3: the main walk's admission workers share one cache (by entity, in 16 locked stripes), so an entity keeps
            // its copy whichever worker admits it; the other walks keep their thread's.
            // I-0077: 64 stripes by a hash of the entity (entities at a fixed stride shared one of the 16 stripes of (entity>>4)&15).
            struct SharedEngineCache {std::array<std::mutex,64> locks;std::array<EngineCache,64> stripes;};static SharedEngineCache shared_engine;
            const size_t stripe=(c.entity*0x9E3779B97F4A7C15ull)>>58;auto& cache=native_admission_sink?shared_engine.stripes[stripe]:engine_cache;
            std::unique_lock<std::mutex> stripe_hold;if(native_admission_sink)stripe_hold=std::unique_lock(shared_engine.locks[stripe]);
            auto& cached=cache.entries[c.entity];
            if(!cached.first || std::memcmp(cached.first.get(),&engine_scratch,sizeof engine_scratch))cached.first=std::make_shared<const NativeSelectedModel::EngineBytes>(engine_scratch);
            cached.second=result.serial;selected.engine_bytes=cached.first;
            if(result.serial>=cache.sweep+120){cache.sweep=result.serial;
                std::erase_if(cache.entries,[&](const auto& item){return item.second.second+120<result.serial;});}};
        const bool engine_bytes_from_record=!native_offscreen_preparing && result.record_epochs.has_value();
        if(!engine_bytes_from_record)share_cached_engine_bytes();
        ingress::CurrentModelPreparationFlags preparation_flags;
        // E03 (I-0105): no animation object: 236290's arguments leave +0xF8 and +0xF9 clear (no kind-8 skip, no special kind 5).
        if(c.effect)selected.preparation_flags=preparation_flags;
        else if(ingress::ReadModelPreparationFlagsAt(c.animation,preparation_flags))selected.preparation_flags=preparation_flags;
        //801: the selected400-byte group is entity+240.1A8580 copies it,
        //then5A120 routes its current+C0/C8 to preparation capability masks.
        std::array<uint64_t,2> shader_capabilities;
        if(c.capability_source){   // CFDE70's stack copy (NativeSelectionCandidate::capability_source)
            if(!Read(c.capability_source,shader_capabilities.data(),sizeof(shader_capabilities))){
                character_outcome="asm_part_unreadable";native_admission_code=2,++result.missing_bindings;NoteNativeSkip("asm_part_unreadable",c.model);return;}
            shader_capabilities[1]|=uint64_t(1)<<63;selected.shader_capabilities=shader_capabilities;}
        else if(Read(c.group+0xC0,shader_capabilities.data(),sizeof(shader_capabilities)))
            selected.shader_capabilities=shader_capabilities;
        Read(model+0xB4,&selected.lod_mask,4);selected.lod_mask|=1u;
        uint32_t lods{};
        for(uint32_t i=0;i<selected.geometry_fades.count;++i){const auto variant=selected.geometry_fades.intents[i].variant;
            if(variant>=0)lods|=1u<<ds3content::SelectOrdinaryModelLod(static_cast<uint32_t>(variant),selected.lod_mask);}
        if(!result.scene_word_ranges)result.scene_word_ranges=NativeShaderWordRanges(result.camera_constants.bytes,result.camera_constants.written);
        const auto& scene_ranges=*result.scene_word_ranges;   // m45 collect cost: once per collect
        const ds3content::ShaderConstantRange view_range{0,result.view_constants.bytes};
        admission_marks.Mark(7);   // m45 profile: engine bytes, flags, capabilities, lods, ranges (section 4 is then the carried tables)
        NativeInstanceBlockTable instance_block_table;   // workstream K P2: this model's instance blocks, per programme and fade
        // C7-1b: the programmes whose instance entries are in the table (owned)
        NativePlannedPrograms planned_instances;
        // m45 collect cost: the carried table is shared as it is (no copy per model) while no programme of the model is new to
        // it; a model that adds entries (a new programme, a fresh admission) copies it first. The planned set is moved out of
        // the carried entry, which the end of the admission replaces (a failed admission leaves the entry without it: its
        // programmes are planned again next time, against the same table).
        std::shared_ptr<const NativeInstanceBlockTable> carried_table;
        const auto own_table=[&]{if(carried_table){instance_block_table=*carried_table;carried_table.reset();}};
        // Review: an admission that ends early after taking the planned set erases the carried entry (its table would have keys
        // no programme owns any more); the end of a complete admission replaces the entry and disarms this.
        bool planned_taken=false;const uintptr_t carried_entity=c.entity;
        auto& instance_carry=native_block_carry.instances;
        struct PlannedGuard {bool& taken;uintptr_t entity;NativeEntityShards<NativeBlockCarry::Instances>& carry;~PlannedGuard(){if(taken){auto& shard=carry.Of(entity);const std::lock_guard hold(shard.lock);shard.map.erase(entity);}}} planned_guard{planned_taken,carried_entity,instance_carry};
        const bool main_view=!native_offscreen_preparing;
        const bool carry_instances=main_view;   // C7-1c
        if(carry_instances){auto& shard=instance_carry.Of(c.entity);const std::lock_guard carry(shard.lock);
            const auto found=shard.map.find(c.entity);
            if(found!=shard.map.end() && found->second.model==model && found->second.count==selected.geometry_fades.count &&
               [&]{for(uint32_t fade=0;fade<selected.geometry_fades.count && fade<2;++fade)
                   if(found->second.records[fade].bytes!=selected.instances[fade].bytes || found->second.records[fade].written!=selected.instances[fade].written)return false;
                   return true;}()){carried_table=found->second.table;planned_instances=std::move(found->second.planned);planned_taken=true;kprofile::Add(kprofile::InstanceTablesCarried);}}
        // C7-2: the model's record, taken out of the store (a failed admission below leaves none); checked, then reused unless
        // this is its revalidation turn (every NativeRecordRevalidationPeriod()-th collect, spread by entity).
        admission_marks.Mark(4);   // m45 profile: engine bytes, flags, ranges, carried instance tables
        const bool records=main_view && result.record_epochs.has_value();
        std::optional<NativeModelRecord> previous;bool revalidate=false;uint64_t records_resets{};
        if(records){
            {auto& shard=native_model_records.entries.Of(c.entity);const std::lock_guard hold(shard.lock);records_resets=native_model_records.resets.load(std::memory_order_relaxed);
                const auto found=shard.map.find(c.entity);
                if(found!=shard.map.end() && found->second){previous=std::move(*found->second);found->second.reset();}}   // the slot stays, empty
            if(previous){const auto outcome=CheckNativeModelRecord(*previous,selected,c,inputs,lods,parameter_animations,*result.record_epochs,&result);
                if(outcome==RecordReused)revalidate=((c.entity*0x9E3779B97F4A7C15ull>>40)+result.serial)%NativeRecordRevalidationPeriod()==0;   // review: the address mixed (pooled entities at a fixed stride would share a turn)
                else{NoteNativeRecordOutcome(outcome);previous.reset();}}
        }
        const bool reuse=previous && !revalidate;
        if(!reuse)++native_fresh_admissions;
        if(engine_bytes_from_record){   // I-0078: the record's copy, renewed in it when the bytes changed (a fresh admission records its own)
            if(previous && previous->engine_bytes && !std::memcmp(previous->engine_bytes.get(),&engine_scratch,sizeof engine_scratch))selected.engine_bytes=previous->engine_bytes;
            else if(previous){selected.engine_bytes=std::make_shared<const NativeSelectedModel::EngineBytes>(engine_scratch);previous->engine_bytes=selected.engine_bytes;}
            else share_cached_engine_bytes();}
        admission_marks.Mark(5);   // m45 profile: the record check
        NativeModelRecord fresh;   // what a fresh admission is recorded under
        // I-0120 B1: the paint view before the meshes bind it (a target made meanwhile differs at the next check).
        if(selected.decal_choice==ingress::ModelDecalChoice::InstanceProvider && selected.patch)
            if(uintptr_t table{};Read(selected.patch,&table,8) && table==image+0x3D41DF0)fresh.paint=NativePaintViewsOf(selected.patch).diffuse;
        auto fresh_outcome=selected.streams || c.cloth_set?RecordStreams:RecordReused;
        std::vector<NativeSelectedMesh> meshes;
        if(reuse){
            // Only the scene, view and instance table entries of the set's programmes remain (C7-1b).
            kprofile::Switch(kprofile::MeshBlocks);
            for(const auto& mesh:previous->meshes)if(mesh.programs && !planned_instances.contains(mesh.programs.get())){own_table();break;}
            uint32_t resolved{};   // I-0078: counted here, added once (eight threads incremented one atomic per recipe)
            for(const auto& mesh:previous->meshes){
                if(!mesh.programs){++result.missing_program_family;continue;}
                PlanNativeProgramTables(mesh.programs,selected,result,instance_block_table,planned_instances,scene_ranges,view_range);
                for(const auto& recipe:mesh.programs->recipes)resolved+=recipe.status==ds3content::ModelProgramStatus::Resolved;}
            result.program_recipes+=resolved;
            kprofile::Add(kprofile::AdmittedMeshes,previous->meshes.size());kprofile::Switch(kprofile::ModelAdmission);
            selected.meshes=previous->meshes;   // a reflection's t59..t62 rebound at prepare (I-0107)
        }else{own_table();meshes.reserve(inputs.meshes.size());}   // C7-1d: no regrowth moves of the meshes
        admission_marks.Mark(6);   // m45 profile: the reused meshes' table plans
        // Lightmap floors: the draw's lightmap set, once per model (its words are kept with the record).
        const auto lightmap_set=reuse?std::nullopt:ReadNativeLightmapSet(c.animation);
        // I-0100 tranche 2 (Anor Londo after a death, Thomas 2026-10-26): what bound this model's lightmaps (t26..t28) at this fresh
        // admission, the worst of its meshes: 1 the draw's set, 2 the material's path (or its packages), 3 SYSTEX_WHITE without a set,
        // 4 SYSTEX_WHITE with a set (its object not resident); 0 no lightmap.
        uint8_t lightmap_source=0;std::string lightmap_name;std::array<float,4> lightmap_constant{-1.f,-1.f,-1.f,-1.f};
        if(!reuse)for(const auto& mesh:inputs.meshes){
            kprofile::Switch(kprofile::MeshLookups);
            const auto& slice=selected.geometry->meshes[mesh.ordinal];
            if(slice.material_id!=mesh.material_id)Refuse("native_selected_geometry_material_membership");
            // Retain current eligibility fields for native pass preparation:
            // only meshes of the LODs the fade intents select (63CB0 +18
            // adjusted by model+B4); others are not reintroduced from storage.
            if(!(mesh.lod_mask&lods))continue;
            NativeSelectedMesh bound;bound.current=mesh;bound.slice=slice;
            {const std::shared_lock hold(model_material_lock);const auto found=model_materials.find(mesh.material);
                if(found==model_materials.end()){character_outcome="missing_material";NoteNativeAsmMissingBinding(c,3,mesh.material);native_admission_code=2,++result.missing_bindings;return;}bound.material=found->second;
                // C7-2: the material bytes this mesh is admitted from, read before the fields below (a later change shows).
                if(records){fresh.materials.push_back(NativeMaterialRecordBytes(*bound.material,mesh.material,result.record_epochs->serial));   // I-0078: the material's kept copy
                    if(!fresh.materials.back())fresh_outcome=RecordMaterial;}
                //This registry also gates retirement of the genuine CPU prefix.
                //Copy current inputs here; the immutable selection retains no
                //borrowed field address and never substitutes initial masks.
                NativeSelectedMesh::MaterialMasks masks;BYTE special{},secondary{};uint32_t mode{};
                if(Read(mesh.material+8,&masks.first,8) && Read(mesh.material+0x10,&masks.second,8) &&
                   Read(mesh.material+0x1C,&masks.secondary,4))bound.material_masks=masks;
                {BYTE no_cast{};bound.no_cast=Read(mesh.material+0xD1,&no_cast,1) && no_cast!=0;}   // D-0180 C2
                NativeSelectedMesh::PosePolicy pose_policy;BYTE cloth_enabled{};
                if(Read(mesh.material+0x22,&cloth_enabled,1) && Read(mesh.material+0x24,&pose_policy.flags,4)){
                    pose_policy.cloth_enabled=cloth_enabled!=0;bound.pose_policy=pose_policy;
                }
                // I-0100 tranche 6: +0xD0 is the GX00 « Decal » (written natively since the GX00 item is parsed). 142130 makes a
                // Decal material's class 0x10 (MTD +0x110, forward) or 8, before its other tests (READ, I-0140 tranche 6,
                // evidence/I-0140/ghidra-decal-class8/): the forward rows 0x29, or the engine-6 G-buffer row, drawn after every
                // engine-5 row (the rows are made pass by pass), with the G-buffer state every class takes, its blend by the
                // material's blend mode (13b). The eyebrows FG_A_2012 (i100-t6i-firelink-o1), the Decal maps M[ARSN]_e, _l_e, _tr,
                // _em_Glow (i100-t6k-firelink-n1).
                if(Read(mesh.material+0xD0,&special,1))bound.current_special=special!=0;
                if(Read(mesh.material+0x250,&mode,4) && Read(mesh.material+0x254,&secondary,1)){
                    bound.base_deformation=mode==1?1u+(secondary!=0):mode==2?3u+(secondary!=0):0u;}
                int32_t index{};if(Read(mesh.material+0x60,&index,4))bound.material_index=index;
                ds3content::ModelWetMaterialParameters wet_parameters;
                if(ingress::ReadModelWetMaterialParameters(mesh.material,wet_parameters))bound.wet_parameters=wet_parameters;
            }
            std::shared_ptr<NativeModelLayout> layout;
            {const std::shared_lock hold(model_layout_lock);const auto found=model_layouts.find(mesh.layout);
                if(found==model_layouts.end()){character_outcome="missing_layout";NoteNativeAsmMissingBinding(c,4,mesh.layout);native_admission_code=2,++result.missing_bindings;return;}layout=found->second;bound.layout=layout;}
            // I-0085: a material retired between the two lookups (its FLVER unloading on the loader thread) has taken its layouts
            // with it, and the prefix may already serve a new material's layout: a miss like a missing layout, not a refusal.
            if(bound.layout->material!=bound.material){bool retired{};
                {const std::shared_lock hold(model_material_lock);const auto found=model_materials.find(mesh.material);retired=found==model_materials.end() || found->second!=bound.material;}
                if(retired){character_outcome="retired_material";NoteNativeAsmMissingBinding(c,4,mesh.layout);native_admission_code=2,++result.missing_bindings;return;}}
            if(bound.layout->material!=bound.material || bound.layout->type!=mesh.layout_type)Refuse("native_selected_mesh_layout_material_mismatch");
            Microsoft::WRL::ComPtr<ID3D12Resource> stream;std::shared_ptr<const void> stream_owner;   // cD5: the binding holds the version
            if(selected.streams){
                // D3FA10 chooses patch by the mesh's +2 identity, not the
                // selected mesh ordinal. Only stream slot0 is overridden.
                for(const auto& candidate:selected.streams->streams)if(candidate.mesh==mesh.patch_index){
                    if(candidate.stride!=slice.stride || candidate.vertices!=slice.vertex_count || !candidate.resource)Refuse("native_selected_stream_extent_mismatch");
                    stream=candidate.resource;stream_owner=selected.streams;break;}
                if(!stream){character_outcome="missing_stream";NoteNativeAsmMissingBinding(c,5,mesh.patch_index);native_admission_code=2,++result.missing_bindings;return;}
            }
            // 1184 (MENUS-MAP 65): a cloth mesh with a published simulated copy draws it as stream 0 (Position,
            // Normal, Tangent in model space; VC_isEnableCloth = 1); stream 1 stays authored.
            uint64_t stream_generation=stream?selected.streams->generation:0;
            if(!stream && c.cloth_set){uintptr_t objects{},objects_end{},object{};
                if(Read(c.cloth_set+0x18,&objects,8) && Read(c.cloth_set+0x20,&objects_end,8) && objects &&
                   mesh.ordinal<(objects_end-objects)/8 && Read(objects+8ull*mesh.ordinal,&object,8) && object){
                    std::shared_ptr<const NativeClothStream> cloth;
                    {const std::lock_guard hold(cloth_stream_lock);const auto found=cloth_streams.find(object);if(found!=cloth_streams.end())cloth=found->second;}
                    if(cloth && cloth->simulated && cloth->resource && ((uint64_t(slice.vertex_count)*slice.stride+3)&~uint64_t(3))==cloth->bytes){
                        stream=cloth->resource;stream_owner=cloth;stream_generation=(1ull<<62)|cloth->generation;bound.cloth_stream=true;
                        // I-0134 (its step review's M1, its second final review's L1): the previous copy only while the current one is
                        // this frame's upload (about 99 % of the admissions in its run); without a step this frame the cloth did not move
                        // against its frame, which the game's programme draws. native_cloth_upload_age counts the ages met.
                        const uint64_t age=native_cloth_frames.load(std::memory_order_relaxed)-cloth->frame;
                        if(cloth->previous_resource && !age)bound.cloth_previous=cloth->previous_resource;
                        static const bool observe_ages=ModuleFlag(L"cloth-velocity-observe.flag");   // its final review's L1: behind its flag
                        static std::atomic<uint64_t> ages[4]{};if(observe_ages)ages[!cloth->previous_resource?3:age<2?age:2].fetch_add(1,std::memory_order_relaxed);
                        static LONG64 age_rows{};if(observe_ages)if(const auto n=InterlockedIncrement64(&age_rows);n<=2 || n%2000==0){char row[200]{};
                            sprintf_s(row,"{\"event\":\"native_cloth_upload_age\",\"age0\":%llu,\"age1\":%llu,\"older\":%llu,\"no_previous\":%llu}\n",
                                static_cast<unsigned long long>(ages[0].load()),static_cast<unsigned long long>(ages[1].load()),static_cast<unsigned long long>(ages[2].load()),
                                static_cast<unsigned long long>(ages[3].load()));Log(row);}}}}
            bound.geometry=BindModelGeometry(slice,selected.geometry->generation,selected.geometry->resource,
                stream_generation,stream,std::move(stream_owner));
            // 5A120's state+2D8 comes from group+98; its bit1 controls the
            // inverted flag6 passed through 140680 to AA280's channel choice.
            kprofile::Switch(kprofile::MeshPrograms);
            bound.programs=AcquireNativeModelPrograms(bound.material,layout,!(selected.fade.group_flags&2u));
            kprofile::Switch(kprofile::MeshTextures);
            PlannedNativeMeshTextures(bound,selected,c,result,*layout);
            ApplyNativeLightmapSet(bound,lightmap_set);
            if(PopinObserve())for(const auto& binding:bound.texture_bindings){const auto texture_slot=binding.input.declaration.slot;if(texture_slot<26 || texture_slot>28)continue;
                const bool white=binding.texture && binding.texture==result.system_textures.white_bound;
                const uint8_t source=white?(lightmap_set?4:3):lightmap_set?1:2;
                if(source>lightmap_source)lightmap_source=source;
                // (tranche 2, the fix 0e89a4dd did not restore the floor after a death: what t26 binds, by name, and its constant)
                if(texture_slot==26 && lightmap_name.empty()){
                    lightmap_name=binding.texture && binding.texture->content?binding.texture->content->name.substr(0,80):"(none)";
                    if(bound.lightmap_constant)lightmap_constant=*bound.lightmap_constant;}}
            // Witness (lightmap floors): the first lightmapped meshes (a t26 binding) of model draw entities, by model name, with their set.
            if(bound.programs && bound.programs->texture_bindings_available){uintptr_t entity_table{};
                if(Read(c.entity,&entity_table,8) && entity_table==image+0x2948EF8 && std::any_of(bound.texture_bindings.begin(),bound.texture_bindings.end(),
                       [](const auto& binding){return binding.input.declaration.slot==26;})){
                    static std::mutex lock;static std::set<std::string> seen;const auto name=NativeModelDataName(model);bool fresh_name=false;
                    {const std::lock_guard hold(lock);if(seen.size()<16)fresh_name=seen.insert(name).second;}
                    if(fresh_name){std::array<uintptr_t,5> objects{};Read(c.animation+0x90,objects.data(),sizeof(objects));std::string clean;
                        for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                        char row[320]{};sprintf_s(row,"{\"event\":\"native_lightmap_entity\",\"model\":\"%.80s\",\"set\":%s,\"objects\":[%d,%d,%d,%d,%d],\"constant\":%s}\n",clean.c_str(),
                            lightmap_set?"true":"false",objects[0]?1:0,objects[1]?1:0,objects[2]?1:0,objects[3]?1:0,objects[4]?1:0,bound.lightmap_constant?"true":"false");Log(row);}}}
            if(bound.programs && bound.programs->policy){
                // 142130 reads this current CPU material-instance flag. Its
                // old template pointer stays nil; native policy supplies data.
                if(bound.current_special.has_value())
                    bound.material_class=ds3content::ClassifyModelMaterial(*bound.programs->policy,*bound.current_special);
            }
            kprofile::Switch(kprofile::MeshMatDyn);
            bound.dynamic_bytes.resize(4);bound.dynamic_written.resize(1);
            bound.parameter_animations_available=parameter_animations.available;
            bound.parameter_animation_rows=parameter_animations.rows.size();
            // 1109: with animation rows (SAT_Equip armour: one row), 22D620 runs 22E5C0 on a copy of the material's dynamic
            // object when the table holds a universal row or one for this material index: each animated descriptor moves
            // towards its row (AnimateModelGeneratedParameters), then 22E930 emits (the embered armour's cinder glow,
            // SAT_Equip_snp_0_float_4_0, TestAnim Reference1903 against Native1903).
            if(bound.programs && bound.programs->generated_parameters && parameter_animations.available){
                const auto& authored=*bound.programs->generated_parameters;
                const bool gate=std::any_of(parameter_animations.rows.begin(),parameter_animations.rows.end(),[&](const auto& row){
                    return row.universal || (bound.material_index && row.material_index==*bound.material_index);});
                std::vector<uint32_t> animated;const auto* words_source=&authored.words;
                if(gate && ds3content::AnimateModelGeneratedParameters(authored,parameter_animations,bound.material->name,animated)){words_source=&animated;
                    static const bool witness=ModuleFlag(L"light-observe.flag");
                    // I-0140 (its final review's L2): a row per (model, material) for its first four distinct first words, 256 pairs at most,
                    // so a cutscene's animated materials show after play's.
                    static std::mutex animated_lock;static std::map<std::pair<uintptr_t,std::decay_t<decltype(bound.material->name)>>,std::set<uint32_t>> animated_seen;bool fresh_animated{};
                    if(witness){const std::lock_guard hold(animated_lock);const std::pair key{model,bound.material->name};
                        if(animated_seen.size()<256 || animated_seen.contains(key)){auto& values=animated_seen[key];
                            fresh_animated=values.size()<4 && values.insert(animated.empty()?0u:animated[0]).second;}}
                    if(fresh_animated){std::string name;
                        for(const auto ch:bound.material->name)if(name.size()<60)name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                        std::string words;for(size_t i=0;i<animated.size() && i<8;++i){char cell[24]{};sprintf_s(cell,"%s%g",i?",":"",std::bit_cast<float>(animated[i]));words+=cell;}
                        char row[480]{};sprintf_s(row,"{\"event\":\"native_generated_animated\",\"model\":\"%.40s\",\"material\":\"%s\",\"offset\":%u,\"words\":[%s]}\n",
                            NativeModelDataName(model).c_str(),name.c_str(),authored.byte_offset,words.c_str());Log(row);}}
                const auto& generated_words=*words_source;
                bound.dynamic_bytes.resize((std::max)(size_t(4),size_t(authored.byte_offset)+generated_words.size()*4));
                bound.dynamic_written.resize(bound.dynamic_bytes.size()/4);
                ds3prod::ProduceMatDynScalars(authored.byte_offset,generated_words.data(),
                    static_cast<uint32_t>(generated_words.size()),bound.dynamic_bytes.data(),
                    static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());
            }
            if(bound.programs && bound.programs->damage_parameters){
                const auto writes=ds3content::ProduceModelDamageParameters(*bound.programs->damage_parameters,
                    parameter_animations,bound.material->name,bound.material_index);
                for(const auto& write:writes){
                    const auto extent=size_t(write.byte_offset)+write.bytes;
                    bound.dynamic_bytes.resize((std::max)(bound.dynamic_bytes.size(),extent));
                    bound.dynamic_written.resize(bound.dynamic_bytes.size()/4);
                    ds3prod::ProduceMatDynScalars(write.byte_offset,write.words.data(),write.bytes/4,
                        bound.dynamic_bytes.data(),static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());
                }
            }
            if(bound.programs && bound.programs->policy){
                bool wet_enabled{};
                if(ds3content::SelectModelWetActivation(bound.programs->shader_family,bound.programs->effect->declarations,
                    result.wet_shader_catalog.get(),wet_enabled)){
                    bound.wet_activation=wet_enabled;
                    std::optional<std::array<float,3>> draw_abc;
                    if(selected.wet_draw_inputs){const auto& value=*selected.wet_draw_inputs;
                        draw_abc=std::array<float,3>{value.a,value.b,value.c};}
                    const auto writes=bound.wet_parameters?ds3content::ProduceModelWetParameters(wet_enabled,*bound.wet_parameters,draw_abc,
                        parameter_animations,bound.material->name,bound.material_index):std::vector<ds3content::ModelParameterWrite>{};
                    for(const auto& write:writes){
                        const auto extent=size_t(write.byte_offset)+write.bytes;
                        bound.dynamic_bytes.resize((std::max)(bound.dynamic_bytes.size(),extent));
                        bound.dynamic_written.resize(bound.dynamic_bytes.size()/4);
                        ds3prod::ProduceMatDynScalars(write.byte_offset,write.words.data(),write.bytes/4,
                            bound.dynamic_bytes.data(),static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());
                    }
                }
            }
            {   // P3 (I-0067 PHANTOMS-SIGNS-NATIVE §2): 22D620/22E930's phantom block, rows 26-30, from the material dynamic
                // object (material+D0) and the draw's ExParam_Phantom* rows (9CAF30 pushes them for a PhantomParam id).
                const auto source=bound.current.material+0xD0;ds3content::ModelPhantomParameters phantom;BYTE flag{};
                // The key globals 4962354.. (categories 8-14) are filled by an initialiser that does not run natively (Native1221:
                // all zero); they hold the inverted CRC-32 of the UTF-16 names, the keys the override rows carry (Native1221).
                static const auto keys=[]{std::array<uint32_t,7> result{};
                    constexpr const wchar_t* names[7]{L"ExParam_PhantomIsEdgeSubtract",L"ExParam_PhantomIsFrontSubtract",L"ExParam_PhantomEdgeColor",
                        L"ExParam_PhantomFrontColor",L"ExParam_PhantomDiffColor",L"ExParam_PhantomSpecColor",L"ExParam_PhantomLightColor"};
                    for(size_t i=0;i<7;++i){uint32_t key=0xFFFFFFFFu;
                        for(const wchar_t* ch=names[i];*ch;++ch)for(unsigned byte=0;byte<2;++byte){key^=(static_cast<uint32_t>(*ch)>>(8*byte))&255u;
                            for(unsigned bit=0;bit<8;++bit)key=(key>>1)^((key&1u)?0xEDB88320u:0u);}
                        result[i]=~key;}
                    return result;}();
                // 141070 initialises this block (flag, zero +0x70..+0xCF, Diff and Spec = the 16 bytes at 143D7FEF0) when
                // gxMtd+0x88 bit 26 (the 43 character shader names of 1447371E0) or the bundle record's resource flag holds;
                // 141070 does not run natively (Native1219: flag clear, rows 26-30 unwritten). Explicit policy: the same
                // initial values for a material whose family has its <family>_Phantom_Fwd programme (A9EF0's alternate slot).
                const bool alternate=bound.programs && (bound.programs->recipes[NativeRecipe(native_phantom_forward_pass,0)].status==ds3content::ModelProgramStatus::Resolved ||
                    bound.programs->recipes[NativeRecipe(native_phantom_forward_pass,1)].status==ds3content::ModelProgramStatus::Resolved);
                bool block=bound.material && Read(source+0x22,&flag,1);
                if(block && flag)block=Read(source+0x70,phantom.subtract.data(),2) && Read(source+0x80,phantom.colours.data(),sizeof(phantom.colours));
                else if(block && alternate){std::array<float,4> unit{};block=Read(image+0x3D7FEF0,unit.data(),16);phantom.colours[2]=phantom.colours[3]=unit;}
                else block=false;
                // 1221 witness: the override rows a phantom-capable material sees (kind, key, universal, index, strength, target).
                if(block && parameter_animations.rows.size()>=7){static LONG64 phantom_rows{};if(InterlockedIncrement64(&phantom_rows)<=4){
                    std::string rows;for(const auto& r:parameter_animations.rows){char cell[160]{};
                        sprintf_s(cell,"%s[%u,%u,%d,%d,%g,%g,%g,%g,%g]",rows.empty()?"":",",r.kind,r.lookup_key,r.universal?1:0,r.material_index,r.strength,
                            r.target[0],r.target[1],r.target[2],r.target[3]);rows+=cell;if(rows.size()>1800)break;}
                    char head[200]{};sprintf_s(head,"{\"event\":\"native_phantom_rows\",\"flag\":%u,\"material_index\":%d,\"keys\":[%u,%u,%u,%u,%u,%u,%u],\"rows\":[",
                        flag,bound.material_index?*bound.material_index:-1,keys[0],keys[1],keys[2],keys[3],keys[4],keys[5],keys[6]);
                    Log((std::string(head)+rows+"]}\n").c_str());}}
                if(block){
                    phantom.enabled=true;
                    for(const auto& write:ds3content::ProduceModelPhantomParameters(phantom,keys,parameter_animations,bound.material->name,bound.material_index)){
                        const auto extent=size_t(write.byte_offset)+write.bytes;
                        bound.dynamic_bytes.resize((std::max)(bound.dynamic_bytes.size(),extent));
                        bound.dynamic_written.resize(bound.dynamic_bytes.size()/4);
                        ds3prod::ProduceMatDynScalars(write.byte_offset,write.words.data(),write.bytes/4,
                            bound.dynamic_bytes.data(),static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());
                    }
                }
            }
            {   // WATER-TESS-WAVE1-READY (22E930's water block, when sub+0x12 is set by GX07): cbMatDynParam +0x110 FC_WaterNormalUvScrollSpeed
                // = sub+0xD0..0xDC, +0x120 FC_WaterDiffuseUvScrollSpeed = +0xE8..+0xF4, +0x130 FC_WaterMiddleUvScrollSpeed = (+0xE0, +0xE4)
                // twice, +0x140 FC_WaterFogParam0 = (0, 0, +0x100, +0x104), +0x150 FC_WaterFogParam1 = (c x c x +0x11C for c = +0x110..+0x118,
                // +0x120), +0x160 FC_WaterFresnel = (+0xFC - +0xF8, +0xF8, +0x124, 1 / a) (the first two zero while the reflection option
                // [[GX2+0x5360]+0x6C] is off), +0x170 FC_WaterFadeParam = (1 / b, 0, 0, 0); a = +0x128 (0.1 when |a| <= FLT_EPSILON), b =
                // +0x12C (a when |b| <= FLT_EPSILON).
                const auto source=bound.current.material+0xD0;BYTE flag{};std::array<float,24> v{};
                if(bound.material && Read(source+0x12,&flag,1) && flag && Read(source+0xD0,v.data(),sizeof(v))){
                    const auto at=[&](uint32_t offset){return v[(offset-0xD0)/4];};
                    uint8_t option{1};{uintptr_t settings{};if(Read(image+0x4961B50,&settings,8) && settings && Read(settings+0x5360,&settings,8) && settings)Read(settings+0x6C,&option,1);}
                    constexpr float epsilon=1.1920929e-7f;   // [143D7ECE8]
                    float a=at(0x128);if(static_cast<float>(std::fabs(static_cast<double>(a)))<=epsilon)a=0.1f;
                    float b=at(0x12C);if(static_cast<float>(std::fabs(static_cast<double>(b)))<=epsilon)b=a;
                    const float f8=option?at(0xF8):0.0f,f9=option?at(0xFC):0.0f,k=at(0x11C);
                    const std::array<float,28> w{at(0xD0),at(0xD4),at(0xD8),at(0xDC),at(0xE8),at(0xEC),at(0xF0),at(0xF4),at(0xE0),at(0xE4),at(0xE0),at(0xE4),
                        0.0f,0.0f,at(0x100),at(0x104),at(0x110)*at(0x110)*k,at(0x114)*at(0x114)*k,at(0x118)*at(0x118)*k,at(0x120),
                        f9-f8,f8,at(0x124),1.0f/a,1.0f/b,0.0f,0.0f,0.0f};
                    std::array<uint32_t,28> words{};for(size_t i=0;i<w.size();++i)words[i]=std::bit_cast<uint32_t>(w[i]);
                    bound.dynamic_bytes.resize((std::max)(bound.dynamic_bytes.size(),size_t(0x180)));bound.dynamic_written.resize(bound.dynamic_bytes.size()/4);
                    ds3prod::ProduceMatDynScalars(0x110,words.data(),static_cast<uint32_t>(words.size()),bound.dynamic_bytes.data(),
                        static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());}
            }
            //140850 sets this word after22D620, including when its generated
            //range overlaps register0. Other unproduced b2 words stay absent.
            ds3prod::ProduceDitherAlpha(bound.dynamic_bytes.data(),static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());
            {   // 22E930 shader-generator values (old host ProduceMatDyn, I-0063,
                // produced): when source+1A, the floats [source+378..380) go to
                // register [source+3B8]; descriptor type 0 is skipped, a colour
                // (type 0x0B, component>=4) expands to scaled RGB plus alpha from
                // [source+398..3A0) with sRGB (descriptor+39==1) raised to 2.2.
                const auto source=bound.current.material+0xD0;BYTE enabled{};uint32_t reg{};
                uintptr_t floats_begin{},floats_end{},colours_begin{},colours_end{},descriptor_owner{},descriptors{};
                if(Read(source+0x1A,&enabled,1) && enabled && Read(source+0x3B8,&reg,4) && reg<4096 &&
                   Read(source+0x378,&floats_begin,8) && Read(source+0x380,&floats_end,8) && floats_begin && floats_end>floats_begin &&
                   (floats_end-floats_begin)%4==0 && (floats_end-floats_begin)/4<=64){
                    std::vector<float> floats((floats_end-floats_begin)/4);std::vector<std::array<uint8_t,8>> colours;
                    bool ok=Read(floats_begin,floats.data(),floats.size()*4);
                    if(ok && Read(source+0x398,&colours_begin,8) && Read(source+0x3A0,&colours_end,8) && colours_begin && colours_end>colours_begin &&
                       (colours_end-colours_begin)%8==0 && (colours_end-colours_begin)/8<=64){
                        colours.resize((colours_end-colours_begin)/8);ok=Read(colours_begin,colours.data(),colours.size()*8);}
                    std::vector<std::array<uint8_t,0x60>> rows;
                    if(ok && Read(source+0x420,&descriptor_owner,8) && descriptor_owner && Read(descriptor_owner+0x18,&descriptors,8) && descriptors){
                        rows.resize((std::min<size_t>)(floats.size(),16));ok=Read(descriptors,rows.data(),rows.size()*0x60);}
                    if(ok){
                        std::vector<float> values;uint32_t colour_index{};bool colour_started=false;
                        for(size_t i=0;i<floats.size();++i){
                            const uint8_t type=i<rows.size()?rows[i][0x38]:0xFF;
                            if(type==0x0B){uint32_t component{};std::memcpy(&component,rows[i].data()+0x30,4);
                                if(component>=4){float colour[4]{1,1,1,1};
                                    // Review: an absent colour array with a started colour group keeps the white default (no colours[0] on an empty vector).
                                    if(!colours.empty() && (colour_started || colour_index<colours.size())){const auto& entry=colours[colour_index<colours.size()?colour_index:0];
                                        colour[0]=entry[2]/255.0f;colour[1]=entry[1]/255.0f;colour[2]=entry[0]/255.0f;colour[3]=entry[3]/255.0f;}
                                    if(rows[i][0x39]==1)for(int q=0;q<3;++q)colour[q]=std::pow(colour[q],2.2f);
                                    float scale=floats[i];if(colour_index<colours.size())std::memcpy(&scale,colours[colour_index].data()+4,4);
                                    for(int q=0;q<3;++q)values.push_back(colour[q]*scale);values.push_back(colour[3]);}
                                else if(component==0){colour_started=true;if(i)++colour_index;}
                                continue;}
                            if(type==0)continue;
                            values.push_back(floats[i]);
                        }
                        const auto extent=size_t(16u*reg)+values.size()*4;
                        if(extent<=65536 && !values.empty()){
                            if(bound.dynamic_bytes.size()<extent){bound.dynamic_bytes.resize((extent+3)&~size_t(3));bound.dynamic_written.resize(bound.dynamic_bytes.size()/4);}
                            ds3prod::ProduceMatDynScalars(16u*reg,reinterpret_cast<const uint32_t*>(values.data()),static_cast<uint32_t>(values.size()),
                                bound.dynamic_bytes.data(),static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());
                        }
                    }
                }
            }
            {   // 22E930 +10 (old host ProduceMatDyn, I-0063): source+38 when the
                // source+10 flag is set, else the per-material buffer's creation
                // zero. Source is the material's inline dynamic object +D0.
                BYTE flag{};uint32_t word{};
                if(Read(bound.current.material+0xD0+0x10,&flag,1) && (!flag || Read(bound.current.material+0xD0+0x38,&word,4))){
                    if(!flag)word=0;
                    if(bound.dynamic_bytes.size()<0x14){bound.dynamic_bytes.resize(0x14);bound.dynamic_written.resize(5);}
                    ds3prod::ProduceMatDynScalars(0x10,&word,1,bound.dynamic_bytes.data(),
                        static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());
                }
            }
            {   // 1068: 22E930's fixed flagged blocks after its generator values
                // (decompiled lines 541-600, 786-800; source = material+D0). A flag set
                // writes the block; a flag clear leaves earlier words (generator) or the
                // per-material buffer's creation zero for words nothing wrote.
                //  +0E: +20 {f,f,f,1} f=+34          +0C|+19: +30 {a,b,-0.5a,0.5-0.5b} from +28/+2C
                //  +0D: +40 32 bytes from +40         +13: +60 {a,-(ab),c,-(cd)} from +130..+13C
                //  +18: +70 16 bytes from +54
                const auto source=bound.current.material+0xD0;std::array<BYTE,0x28> flags{};
                if(Read(source,flags.data(),flags.size())){
                    const auto block=[&](uint32_t offset,bool set,const std::array<float,4>& values){
                        if(bound.dynamic_bytes.size()<offset+16){bound.dynamic_bytes.resize(offset+16);bound.dynamic_written.resize((offset+16)/4);}
                        if(set){if(std::all_of(values.begin(),values.end(),[](float v){return std::isfinite(v);}))
                            ds3prod::ProduceMatDynScalars(offset,reinterpret_cast<const uint32_t*>(values.data()),4,bound.dynamic_bytes.data(),
                                static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());return;}
                        for(uint32_t word=0;word<4;++word)if(!bound.dynamic_written[offset/4+word]){const uint32_t zero{};
                            ds3prod::ProduceMatDynScalars(offset+4*word,&zero,1,bound.dynamic_bytes.data(),
                                static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());}
                    };
                    std::array<float,4> v{};
                    {float f{};const bool set=flags[0x0E] && Read(source+0x34,&f,4);block(0x20,set,{f,f,f,1.f});}
                    {const auto mask=NativeBlendMaskParam(bound.current.material);block(0x30,mask.has_value(),mask?*mask:std::array<float,4>{});}
                    {std::array<float,8> pom{};const bool set=flags[0x0D] && Read(source+0x40,pom.data(),32);   // I-0113: 22E930 copies 32 bytes
                        block(0x40,set,{pom[0],pom[1],pom[2],pom[3]});block(0x50,set,{pom[4],pom[5],pom[6],pom[7]});}
                    {const bool set=flags[0x13] && Read(source+0x130,v.data(),16);block(0x60,set,{v[0],-(v[0]*v[1]),v[2],-(v[2]*v[3])});}
                    {const bool set=flags[0x18] && Read(source+0x54,v.data(),16);block(0x70,set,v);}
                    // I-0105 (Lightning Spear's s03000, GXFlver_ColZFeather: its PS reads +F0 FC_SoftVolume): +16: +F0 16 bytes from
                    // +320; +17: +100 16 bytes from +330.
                    {const bool set=flags[0x16] && Read(source+0x320,v.data(),16);block(0xF0,set,v);}
                    {const bool set=flags[0x17] && Read(source+0x330,v.data(),16);block(0x100,set,v);}
                    // I-0113 tranche 7: +1C: +D0 FC_FlowMapUvTiling 16 bytes from +300 (GX36); +1D: +E0 FC_FlowMapIntencity from +310 (GX37).
                    {const bool set=flags[0x1C] && Read(source+0x300,v.data(),16);block(0xD0,set,v);}
                    {const bool set=flags[0x1D] && Read(source+0x310,v.data(),16);block(0xE0,set,v);}
                    // I-0105 tranche 9 (the surface snow, PRODUCERS-UNION-SNOW-GLOW 2.3): +11 (141070's GX06): +A0 FC_SnowMaskParam
                    // {s, 0, -0.5 s, 0}, s = +3C (GX06's first value).
                    {float s{};const bool set=flags[0x11] && Read(source+0x3C,&s,4);block(0xA0,set,{s,0.f,-0.5f*s,0.f});}
                    {   // I-0105 (the burn look, BURN.md): +19 (GX13): +C0 FC_BlendRate {d, 0, 0, 0}, d = +30 (GX13's third word) moved
                        // toward the target of the draw's kind-7 parameter row by its strength (22D620 with 146850(rows, name, 7)): the
                        // burn update 667430 writes that row for the materials of the Burn MTDs while an object burns, removes it after.
                        // 22D620's outer gate (a universal row, or the row's name CRC +0x78 = the material's +0x60) is implied: 29CF0 keys
                        // the row by the material's name, whose CRC is +0x60 (1B0AB0). An unavailable snapshot gives the material's own
                        // value, as 22D620 with no owner.
                        float d{};const bool set=flags[0x19] && Read(source+0x30,&d,4);ds3content::ModelParameterAnimation burn;
                        if(set && ds3content::SelectModelParameterAnimation(parameter_animations,bound.material->name,0,burn,7) && burn.target)
                            d+=((*burn.target)[0]-d)*burn.strength;
                        block(0xC0,set,{d,0.f,0.f,0.f});}
                    {   // 1108 (140850): with material+22 (cloth enabled), +80 VC_isEnableCloth (uint) = draw state +180
                        // bit 8 set and bit 10 clear. Native has no draw state +180: explicit open policy, 1 exactly
                        // when the native pose of this draw carries a cloth reference (pose.cloth), else 0.
                        // 1110: with material+22 clear 140850 leaves +80 unwritten: the per-material buffer's creation zero.
                        BYTE cloth_enabled{};
                        if(Read(bound.current.material+0x22,&cloth_enabled,1)){
                            // 1184 (MENUS-MAP 65): 2362C70 binds the simulated copy only once the instance flag CI+0x62
                            // is set; the cloth VS then uses VC_World alone. Without a bound copy the mesh is skinned by
                            // its palette (enable 0) instead of drawing its bind pose.
                            const uint32_t enable=cloth_enabled && bound.cloth_stream?1u:0u;
                            if(bound.dynamic_bytes.size()<0x84){bound.dynamic_bytes.resize(0x84);bound.dynamic_written.resize(0x84/4);}
                            ds3prod::ProduceMatDynScalars(0x80,&enable,1,bound.dynamic_bytes.data(),
                                static_cast<uint32_t>(bound.dynamic_bytes.size()),bound.dynamic_written.data());}}
                    {   // 1096 (22E930 asm 14022FF53-14023002A): +20 -> +180 FC_DynamicDiffuseColor. Provider
                        // p=[+360] absent: the 16 bytes at 4929940. Else colour = [+350..+35C] when p+54 and
                        // object+1E are set, otherwise *p->vt+70(p); scale = [+35C] when p+41 and object+1F are
                        // set, otherwise [p+50]; result {s*r,s*g,s*b,[492994C]} (object = source here).
                        std::array<float,4> colour{};const bool set=NativeDynamicDiffuseColour(source,flags,bound.material.get(),colour);
                        // m50 sky (Native1872/1874, Reference1873): the flag +0x20 was clear natively because 141070 (GX14, 22BD90) and
                        // its MTD helper 22BEC0 did not run (both transcribed at the material's creation); the original binds every
                        // material's own block (141F80 allocates the per-frame images for every material: Fable review 2026-10-17).
                        block(0x180,set,colour);}
                    {   // 1093 (22E930 asm 14023002A-1402300A0): +21 -> +190 FC_SSS_ID {float(id),0,0,0}.
                        // id: provider p=[+370] (none: 0.0); if p's vt+30 query is true and [+368] is set,
                        // [[+368]+30], else [[p+10]+30] (uint, converted as CVTSI2SS of the zero-extended word).
                        bool set=flags[0x21]!=0;float id{};
                        if(set){uintptr_t provider{};
                            if(!Read(source+0x370,&provider,8))set=false;
                            else if(provider){uintptr_t table{},query{},chosen{},alternate{};uint32_t word{};
                                set=Read(provider,&table,8) && Read(table+0x30,&query,8) && query>=image && query<image+0x2800000;
                                if(set){const bool ready=reinterpret_cast<bool(__fastcall*)(uintptr_t)>(query)(provider);
                                    set=Read(source+0x368,&alternate,8);
                                    if(set && ready && alternate)chosen=alternate;else if(set)set=Read(provider+0x10,&chosen,8) && chosen;
                                    if(set)set=Read(chosen+0x30,&word,4);if(set)id=static_cast<float>(word);}}}
                        // 1141: natively 22BF70's link never exists (Native1140: source+21 clear and +370 null for every
                        // SSS material); the native profile registry supplies the linked profile's index.
                        if(!set && !flags[0x21] && bound.material)if(const auto profile=NativeSssProfileOf(*bound.material)){set=true;id=static_cast<float>(*profile);}
                        block(0x190,set,{id,0.f,0.f,0.f});}
                }
            }
            kprofile::Switch(kprofile::MeshBlocks);
            if(bound.programs)for(const auto& parameter:bound.programs->numeric_parameters){
                ds3content::ModelParameterAnimation animation;ds3content::ModelParameterWrite write;
                if(ds3content::SelectModelParameterAnimation(parameter_animations,bound.material->name,
                    parameter.declaration.lookup_key,animation,parameter.declaration.animation_selector) &&
                    ds3content::ProduceModelParameter(parameter,animation,write))
                    bound.numeric_writes.push_back(write);
            }
            if(bound.programs)PlannedNativeMeshBlocks(bound,selected,result,instance_block_table,planned_instances,scene_ranges,view_range);
            if(!bound.programs)++result.missing_program_family;
            else for(const auto& recipe:bound.programs->recipes)
                if(recipe.status==ds3content::ModelProgramStatus::Resolved)++result.program_recipes;
            if(records && fresh_outcome==RecordReused)fresh_outcome=NoteNativeRecordMesh(bound,
                std::span<const uint8_t>(*fresh.materials.back()),fresh);
            meshes.push_back(std::move(bound));kprofile::Add(kprofile::AdmittedMeshes);
            kprofile::Switch(kprofile::ModelAdmission);
        }
        kprofile::Switch(kprofile::ModelAdmission);
        if(!reuse)selected.meshes.items=std::make_shared<const std::vector<NativeSelectedMesh>>(std::move(meshes));
        if(selected.meshes.empty()){native_admission_code=5,++result.no_meshes;return;}
        // 1103: a character part model keeps the construction words +220/+224 natively (0xFFFF, 0): under the
        // original renderer they hold the union of its material classes and policy flags (Reference1102), which
        // 57A20 tests for the engine5 split and engine7 rows. Produce both from the native classification.
        // C1 (I-0068 NPC-FAMILIES-STATIC §2.1, PRODUCERS-UNION §1.4): every model, not only character and menu parts:
        // without it, SprjModelDrawEntity characters (37 of 157, 202 class-0x100 meshes) and map pieces never get the
        // engine-7 or 0x1F rows. The pose owners' auxiliary policy (model_policy_flags) keeps its part-only scope.
        // I-0078 (i74-native-75: a fifth of a reused model's admission went to this section): a reused record's union, kept with it.
        std::optional<std::array<uint32_t,2>> material_union;
        if(reuse && previous->material_union_known)material_union=previous->material_union;
        else{uint32_t classes{},flags{};bool complete=true;
            for(const auto& mesh:selected.meshes){if(!mesh.material_class || !mesh.programs || !mesh.programs->policy){complete=false;break;}
                classes|=*mesh.material_class;flags|=mesh.programs->policy->flags;}
            if(complete)material_union=std::array<uint32_t,2>{classes,flags};}
        {if(material_union){const auto [classes,flags]=*material_union;selected.material_mask=classes;selected.material_flags=flags;
                if((result.regions || native_offscreen_preparing))NoteNativeRegionClasses(c.entity,model,classes,flags,result.serial);   // I-0113 tranches 14, 35
                if(c.neutral_instance_decal || (selected.character && selected.patch)){const std::lock_guard policy_hold(model_policy_lock);
                    if(c.neutral_instance_decal)model_policy_flags[model]=flags;
                    if(selected.character && selected.patch){if(native_patch_flags.size()>=4096)native_patch_flags.clear();native_patch_flags[selected.patch]=flags;}}}}
        // Keep one resource/producer snapshot per entity, with separate pass
        // fade intents. LOD0 storage does not qualify other requested variants.
        selected.instance_blocks=carried_table?carried_table:std::make_shared<const NativeInstanceBlockTable>(std::move(instance_block_table));   // C7-1
        // The table's keys stay owned: planned holds every programme it was filled for (the carried ones, copied with the
        // table, and this collect's; the record goes with its entity after 120 collects unseen).
        // m45 collect cost: an entry whose table is still shared (no programme of the model was new; its model, count and records
        // matched) takes its planned set back in place, without copying the records into a new entry.
        if(carry_instances){auto& shard=instance_carry.Of(c.entity);const std::lock_guard carry(shard.lock);
            const auto found=carried_table?shard.map.find(c.entity):shard.map.end();
            if(found!=shard.map.end() && found->second.table==carried_table){
                found->second.planned=std::move(planned_instances);found->second.serial=result.serial;}
            else shard.map.insert_or_assign(c.entity,NativeBlockCarry::Instances{model,selected.geometry_fades.count,
                selected.instances,selected.instance_blocks,result.serial,std::move(planned_instances)});
            planned_taken=false;}
        admission_marks.Mark(8);   // m45 profile: the material classes, the instance blocks and the carried entry
        // I-0100 tranche 2: one popin_native_lightmap row when the source of a model's lightmaps changes between its fresh admissions.
        if(!reuse && lightmap_source && main_view && PopinObserve()){static std::mutex lock;static std::unordered_map<uint64_t,std::string> sources;bool changed{};
            // A row when the source, the t26 texture or its constant change between fresh admissions, and again after each scene reset.
            const uint64_t key=uint64_t(c.entity)^(uint64_t(model)<<1);
            char state[160]{};sprintf_s(state,"%u|%u|%llu|%s|%g,%g,%g,%g",lightmap_source,lods,static_cast<unsigned long long>(NativeKeptDrawResets()),lightmap_name.c_str(),
                lightmap_constant[0],lightmap_constant[1],lightmap_constant[2],lightmap_constant[3]);
            {const std::lock_guard hold(lock);if(sources.size()>65536)sources.clear();auto& last=sources[key];changed=last!=state;last=state;}   // review: bounded
            if(changed){static const char* const names[]{"","set","path","white","white_set"};const auto place=PopinPlaceAt(c.placement.data(),0);char row[480]{};
                sprintf_s(row,"{\"event\":\"popin_native_lightmap\",\"serial\":%llu,\"entity\":\"%llx\",\"model\":\"%s\",\"source\":\"%s\",\"t26\":\"%s\",\"constant\":[%g,%g,%g,%g],\"lods\":%u,\"scene\":%llu,\"d\":%.2f}\n",
                    static_cast<unsigned long long>(result.serial),static_cast<unsigned long long>(c.entity),PopinName(model).c_str(),names[lightmap_source],lightmap_name.c_str(),
                    lightmap_constant[0],lightmap_constant[1],lightmap_constant[2],lightmap_constant[3],lods,static_cast<unsigned long long>(NativeKeptDrawResets()),place.d);Log(row);}}
        if(records){
            if(reuse){NoteNativeRecordOutcome(RecordReused);previous->serial=result.serial;}
            else{
                bool gave_up{};for(size_t m=0;m<selected.meshes.size() && !gave_up;++m)gave_up=selected.meshes[m].texture_gave_up;
                if(previous && !gave_up){   // the revalidation: the fresh admission against the record, mesh by mesh (I-0120 C1: not one with a lookup given up)
                    NoteNativeRecordOutcome(RecordRevalidated);const char* field=nullptr;size_t at=0;
                    if(previous->meshes.size()!=selected.meshes.size())field="mesh_count";
                    for(;!field && at<selected.meshes.size();++at)field=NativeMeshDifference(previous->meshes[at],selected.meshes[at]);
                    if(field){NoteNativeRecordOutcome(RecordMismatch);static LONG64 rows{};if(InterlockedIncrement64(&rows)<=32){char row[256]{};
                        sprintf_s(row,"{\"event\":\"native_model_record_mismatch\",\"model\":%llu,\"mesh\":%zu,\"field\":\"%s\"}\n",
                            static_cast<unsigned long long>(model),at?at-1:0,field);Log(row);}}
                    // m45 frame cost: a record the revalidation found equal keeps its mesh set (what the reuse path takes), so the
                    // prepare's kept draws of the model stay valid (Native1840: 52 revalidations a collect rebuilt their rows).
                    else selected.meshes=previous->meshes;
                }
                previous.reset();   // a fresh admission replaces the record (or leaves none)
                uintptr_t patch_table{};
                if(fresh_outcome==RecordReused && selected.patch && !Read(selected.patch,&patch_table,8))fresh_outcome=RecordModel;
                if(fresh_outcome!=RecordReused)NoteNativeRecordOutcome(fresh_outcome);
                else{
                    fresh.model=model;fresh.geometry=selected.geometry;fresh.storage=storage;fresh.lods=lods;fresh.prefer_channel=!(selected.fade.group_flags&2u);
                    fresh.neutral_instance_decal=selected.neutral_instance_decal;fresh.decal_choice=selected.decal_choice;
                    fresh.patch=selected.patch;fresh.patch_table=patch_table;fresh.wet_draw_inputs=selected.wet_draw_inputs;
                    fresh.parameter_animations=parameter_animations;fresh.inputs=inputs.meshes;fresh.epochs=*result.record_epochs;
                    if(lightmap_set){const auto& objects=lightmap_set->objects;   // review: only for a model with a set   // lightmap floors: the set's five words
                        for(size_t k=0;k<objects.size();++k){fresh.lightmap_words.emplace_back(c.animation+0x90+8*k,objects[k]);uintptr_t table{},inner{};   // review: a holder's texture too
                            if(objects[k] && Read(objects[k],&table,8) && table==image+0x29559E8 && Read(objects[k]+0x10,&inner,8))fresh.lightmap_words.emplace_back(objects[k]+0x10,inner);}}
                    fresh.meshes=selected.meshes;fresh.serial=result.serial;
                    fresh.engine_bytes=selected.engine_bytes;fresh.material_union_known=true;fresh.material_union=material_union;   // I-0078
                    previous=std::move(fresh);NoteNativeRecordOutcome(RecordBuilt);
                }
            }
            if(previous){auto& shard=native_model_records.entries.Of(c.entity);const std::lock_guard hold(shard.lock);
                if(native_model_records.resets.load(std::memory_order_relaxed)==records_resets)shard.map[c.entity]=std::move(*previous);}   // in its slot when it has one
        }
        admission_marks.Mark(9);   // m45 profile: the record store
        kprofile::Switch(kprofile::None);NoteNativeModelReuse(selected,result.serial);kprofile::Switch(kprofile::ModelAdmission);   // C-5 kept out of C-3a
        admission_marks.Mark(10);   // m45 profile: the reuse note (NoteNativeModelReuse)
        if(native_admission_sink)slot_guard.kept=true;   // I-0071 point 3, I-0077, I-0091: built in its slot
        else result.models.push_back(std::move(selected));
        kprofile::Add(kprofile::AdmittedModels);
        admission_marks.Mark(11);   // m45 profile: the push of the model
    }catch(...){if(control->stop)return;Refuse("native_scene_selection_admission_failed");}
}
// 1185 (MENUS-MAP 65-66): from a cloth instance field I (DD5FD0 object) to the dynamic mesh set: CI = [I+0x38] (the
// Havok cloth object, 2362C70), set = [CI+0x38] (23633A0, one per instance; set+0x08 is the FLVER's shared GX model,
// so the per-instance link goes through I). Accepted when the set's mesh vector matches the drawn model's mesh count.
// Kinds: 0 SprjModelDrawEntity (E+0xA70), 1 ChrAsm part (descriptor+0x18), 2 SprjFD4ModelItem (item+0x750).
uintptr_t NativeClothSetOf(uintptr_t field,uintptr_t model,int kind,uintptr_t* instance_out=nullptr) noexcept {
    uintptr_t instance{},havok{},set{},begin{},end{};int32_t meshes{};
    static LONG64 found[3]{},refused[3]{};
    if(!Read(field,&instance,8) || !instance || !Read(instance+0x38,&havok,8) || !havok || !Read(havok+0x38,&set,8) || !set)return 0;
    const bool valid=Read(set+0x18,&begin,8) && Read(set+0x20,&end,8) && Read(model+0xA0,&meshes,4) && meshes>0 && end>=begin &&
        (end-begin)/8==static_cast<uint64_t>(meshes);
    const auto n=native_rt_gates?0:InterlockedIncrement64(valid?&found[kind]:&refused[kind]);   // I-0117 T1 step review L3: not the rays' walk
    if(n==1 || (n && n%300==0)){   // 1185 witness: the simulation flag CI+0x62 (2362D80) and a fingerprint of the first CPU copy
        BYTE simulated{},enabled{};Read(havok+0x62,&simulated,1);Read(havok+0x60,&enabled,1);
        uintptr_t copies{},copy{};uint32_t bytes{};uint64_t fingerprint{1469598103934665603ull};
        if(Read(set+0x38,&copies,8) && copies && Read(copies+4,&bytes,4) && Read(copies+8,&copy,8) && copy){
            std::array<BYTE,256> head{};if(Read(copy,head.data(),std::min<size_t>(head.size(),bytes)))for(const auto b:head)fingerprint=(fingerprint^b)*1099511628211ull;}
        std::array<float,16> m50{},m90{};Read(instance+0x50,m50.data(),64);Read(instance+0x90,m90.data(),64);
        std::string matrices;for(const auto v:m50){char cell[24]{};sprintf_s(cell,"%s%g",matrices.empty()?"":",",v);matrices+=cell;}
        matrices+="],\"i90\":[";bool first=true;for(const auto v:m90){char cell[24]{};sprintf_s(cell,"%s%g",first?"":",",v);first=false;matrices+=cell;}
        char row[1200]{};sprintf_s(row,"{\"event\":\"native_cloth_set_link\",\"kind\":%d,\"valid\":%s,\"count\":%lld,\"vector\":%llu,\"meshes\":%d,\"ci62\":%u,\"ci60\":%u,\"copy_fingerprint\":\"%016llX\",\"i50\":[%s]}\n",
        kind,valid?"true":"false",n,static_cast<unsigned long long>(end>=begin?(end-begin)/8:0),meshes,simulated,enabled,static_cast<unsigned long long>(fingerprint),matrices.c_str());Log(row);}
    if(valid && instance_out)*instance_out=instance;
    return valid?set:0;
}
std::atomic<uint64_t> native_disp_children{},native_disp_items{},native_disp_unsupported_children{},native_asm_parts{};   // I-0077: counted by the walk's workers
void __fastcall AdmitNativeSelectedObject(NativeSelectionVisitor* visitor,uintptr_t entity) noexcept {
    const kprofile::Scope k_filter(kprofile::EntityFilter);
    // Selection racing native close: retired owners are no longer admitted.
    if(control->stop)return;
    // I-0117 T2 (T1's last step review; FINISH-LINE line 1): in the rays' walk an entity whose visit fails (its children's bound,
    // an exception) is skipped alone: the candidates it had gathered are taken back, the entity named, the walk goes on.
    // I-0137 tranche 6 (ROADMAP workstream 4 step 2): the main walk too, wherever its candidates are deferred (every walk of
    // 12_scene_collect's; only a direct admission, `deferred` null, still refuses).
    const size_t rt_mark=visitor->deferred?visitor->deferred->size():0;
    const auto rt_skip=[&](const char* why) noexcept {if(visitor->deferred && visitor->deferred->size()>rt_mark)
        visitor->deferred->erase(visitor->deferred->begin()+static_cast<ptrdiff_t>(rt_mark),visitor->deferred->end());NoteNativeSkip(why,entity);};
    try {
        auto& result=*visitor->result;
        // T5b step review M2: the rays' traversers (`uncounted`) still count their visits (the layers' order by the last walk's).
        if(!visitor->expanding){++visitor->objects;if(!visitor->uncounted && ++result.objects>32768)Refuse("native_scene_selection_object_bound");}
        const auto pointer=[](uintptr_t address){uintptr_t value{};
            if(!Read(address,&value,8)){NoteNativeSkip("pointer_unreadable",address);return uintptr_t{};}return value;};   // I-0101: 0 skips
        // I-0100 tranche 2 (T2a, popin-observe.flag): the gate that ends an entity in the main walk (kind << 4 | gate, as
        // NativeSelectedScene::popin_gates), for the objects the original draws and the native never yields, and the characters
        // the native draws on some frames only (Archdragon Peak, Thomas 2026-10-26 05:12).
        // I-0100 tranche 4 (T2b, NativeCullMargin): in the main view's walk, a draw entity the widened culler passed that the view's
        // own culler would have ended (popin-observe.flag only).
        const auto popin_gate=[&](uint8_t code){
            if(native_rt_gates)++(*native_rt_gates)[code&63];
            if(!PopinObserve() || visitor->rays_walk || native_offscreen_preparing)return;
            try{const std::lock_guard hold(result.popin_gates_lock);result.popin_gates.emplace_back(entity,code);}catch(...){}};
        const auto margin_kept=[&](int inside_word,bool unset){return PopinObserve() && native_cull_margin.active && !visitor->rays_walk &&
            !native_offscreen_preparing && !inside_word && !unset &&
            !reinterpret_cast<int(__fastcall*)(uintptr_t,uintptr_t)>(image+0x06F310)(native_cull_margin.real,entity+0xB80);};
        const auto entity_table=pointer(entity);
        if(!entity_table){NoteNativeSkip("entity_table_null",entity);return;}   // final review L6: unreadable or null, not an unsupported class
        const auto classify=reinterpret_cast<int(__fastcall*)(uintptr_t,uintptr_t)>(image+0x06F310);
        int inside{};std::memcpy(&inside,visitor->base.data()+0x24,4);
        uintptr_t culler{};std::memcpy(&culler,visitor->base.data()+0x18,8);
        // I-0077 (ER 141B4CF40): in the main walk, only the tests that need the walk's bounds stack (the frustum classification of
        // a map piece's E+0x40 and a draw entity's E+0xB80, skipped under a fully inside node), then the entity is gathered; the
        // rest of this function runs for it on the pool with `expanding` set and the inside word 1 (the same tests, already passed).
        if(visitor->gather){
            if(entity_table==image+0x29489A0){if(!inside && !classify(culler,entity+0x40)){popin_gate(1);return;}}
            else if(entity_table==image+0x2948EF8){
                if(pointer(pointer(entity)+0x98)!=image+0xD07E10){NoteNativeSkip("entity_bounds_identity",entity);return;}   // I-0101
                bool bounds_unset=false;alignas(16) float box[8]{};const bool box_read=Read(entity+0xB80,box,sizeof box);
                if(box_read){bounds_unset=true;for(const auto v:box)if(v!=0.f){bounds_unset=false;break;}}
                if(!inside && !bounds_unset && !classify(culler,entity+0xB80)){popin_gate(0x11);return;}
                if(margin_kept(inside,bounds_unset))popin_gate(0x16);}
            visitor->gather->push_back(entity);return;}
        if(entity_table==image+0x29489A0){
            // SprjModelDispEntity (map piece): D04B90's eligibility on its own
            // fields, bounds 2D6E0 = entity+40, then every SprjFD4ModelItem child.
            // m50 sky (WAVE1-LAST-EFFECTS-STATIC-PREP 3.4, S1): the first 80 map pieces the main walk refuses, by gate, with their
            // children's model names (1 frustum, 2 flag +0x388 bit 0, 3 visibility words, 4 category mask).
            static const bool disp_diagnostics=ModuleFlag(L"light-observe.flag");   // review: diagnostics only with light-observe.flag
            const auto refuse_disp=[&](int reason){if(!disp_diagnostics || native_offscreen_preparing || native_rt_gates)return;
                // the frustum refusals only for bounds wider than 1 km (sky-sized; +0x40 minimum, +0x50 maximum, INFERRED layout)
                if(reason==1){float bounds[8]{};if(!Read(entity+0x40,bounds,sizeof(bounds)) || !(bounds[4]-bounds[0]>1000.f || bounds[6]-bounds[2]>1000.f))return;}
                static std::mutex lock;static std::set<uintptr_t> seen;
                {const std::lock_guard hold(lock);if(seen.size()>=200 || !seen.insert(entity).second)return;}
                std::string names;uintptr_t head{},node{};uint32_t walked{};if(Read(entity+0x90,&head,8) && head)Read(head,&node,8);
                for(;node && node!=head && walked<8;++walked){uintptr_t item{},model{};if(Read(node+0x10,&item,8) && item && Read(item+0x58,&model,8) && model){
                    for(const auto ch:NativeModelDataName(model))names.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');names+=' ';}if(!Read(node,&node,8))break;}
                uint64_t bits{};Read(entity+0x20,&bits,8);uint32_t flags{};Read(entity+0x388,&flags,4);
                char row[400]{};sprintf_s(row,"{\"event\":\"native_disp_refused\",\"reason\":%d,\"bits\":%llu,\"flags\":%u,\"models\":\"%.240s\"}\n",reason,
                    static_cast<unsigned long long>(bits&0xFF),flags,names.c_str());Log(row);};
            if(!inside && !classify(culler,entity+0x40)){refuse_disp(1);popin_gate(1);return;}
            // Diagnostic (D-0180 C4 casters): the category byte (+0x20) of the map pieces the main walk visits (native_disp_bits).
            static const bool census=ModuleFlag(L"light-observe.flag");
            if(census && !native_offscreen_preparing && !native_rt_gates){static volatile LONG64 bits_seen[256]{},visits{};uint64_t bits{};Read(entity+0x20,&bits,8);
                InterlockedIncrement64(&bits_seen[bits&0xFF]);
                if(InterlockedIncrement64(&visits)%30000==0){std::string row="{\"event\":\"native_disp_bits\",\"bits\":{";bool first=true;
                    for(int b=0;b<256;++b)if(bits_seen[b]){row+=(first?"\"":",\"")+std::to_string(b)+"\":"+std::to_string(bits_seen[b]);first=false;}
                    row+="}}\n";Log(row.c_str());}}
            uint32_t flags{};if(!Read(entity+0x388,&flags,4)){NoteNativeSkip("disp_policy_unreadable",entity);return;}   // I-0101
            if(!(flags&1)){refuse_disp(2);popin_gate(2);return;}
            alignas(16) std::array<uint32_t,8> words{},visible{};alignas(16) std::array<uint32_t,4> mask{},category{};
            reinterpret_cast<void(__fastcall*)(uintptr_t,void*)>(image+0xD03050)(entity,words.data());
            const auto live=reinterpret_cast<void*(__fastcall*)(uintptr_t,void*,void*)>(image+0x64F000)(entity+0x39C,visible.data(),words.data());
            if(!reinterpret_cast<BYTE(__fastcall*)(void*)>(image+0x619800)(live)){refuse_disp(3);popin_gate(3);return;}
            uintptr_t service{};if(!Read(image+0x4796298,&service,8) || !service){if(native_rt_gates){native_rt_walk_failed=true;return;}Refuse("native_scene_selection_disp_service");}
            reinterpret_cast<void(__fastcall*)(uintptr_t,void*)>(image+0xDFD8D0)(service,mask.data());
            const auto allowed=reinterpret_cast<void*(__fastcall*)(uintptr_t,void*,void*)>(image+0xD043C0)(entity+0x38C,category.data(),mask.data());
            if(!reinterpret_cast<BYTE(__fastcall*)(void*)>(image+0xD04350)(allowed)){refuse_disp(4);popin_gate(4);return;}
            popin_gate(0);
            if(!visitor->rays_walk)NoteNativeMarginPiece(entity);   // I-0141 t22 (popin-observe.flag): an admitted piece, gathered or whole filter
            if(disp_diagnostics){float bounds[8]{};if(Read(entity+0x40,bounds,sizeof(bounds)) && (bounds[4]-bounds[0]>1000.f || bounds[6]-bounds[2]>1000.f))refuse_disp(0);}   // admitted, sky-sized
            const uintptr_t head=pointer(entity+0x90);if(!head)return;
            uint32_t walked=0;
            for(uintptr_t node=pointer(head);node && node!=head;node=pointer(node)){
                // I-0117: the rays' walk skips the entity; I-0137 tranche 6: a map piece's child list running past 4,096 (a list that never
                // comes back to its head) is the game's data, so every walk whose candidates are deferred takes back those this entity
                // gave and skips it; only a direct admission (the menu's offscreen view) still refuses
                if(++walked>4096){if(native_rt_gates || visitor->deferred){rt_skip(native_rt_gates?"rt_walk_disp_children_bound":"disp_children_bound");return;}
                    Refuse("native_scene_selection_disp_children_bound");}
                const auto item=pointer(node+0x10);if(!item)continue;
                const bool tally=!native_rt_gates;   // I-0117 T1 final review L1: the main view's totals, not the rays' walk
                if(tally)native_disp_children.fetch_add(1,std::memory_order_relaxed);
                if(pointer(item)!=image+0x2951FF8){if(tally)native_disp_unsupported_children.fetch_add(1,std::memory_order_relaxed);continue;}
                if(tally)native_disp_items.fetch_add(1,std::memory_order_relaxed);
                NativeSelectionCandidate candidate;
                candidate.entity=item;candidate.model=pointer(item+0x58);if(!candidate.model)continue;
                candidate.group=entity+0x1F0;candidate.animation=item+0x70;candidate.source_field=item+0x740;
                // D3DA20 descriptor+20 = [item+6E8] (D079C0 supplies entity+A40): the parameter animation owner.
                if(!Read(item+0x6E8,&candidate.parameter_animations,8)){NoteNativeSkip("item_pointer_unreadable",item);continue;}   // I-0101: not "absent"
                // I-0140 (the ending's embers): D3DA20 then lets the item's components fill the descriptor ((item+0x18)->vt+0x60, 140D28D80);
                // the « MaterialParamModifier » component at [item+0x50] (vtable 294FF98, made by a Remo act through 140D29220) sets +0x20 to
                // its own modifier X+8 (140D2A790: lea rax,[rcx+8]; mov [rdx+0x20],rax), whose table holds the Remo MaterialParam rows
                // (140D2A6F0; e.g. the universal « CinderIntensityScale » of 140CC0720): the parameter owner is then that modifier.
                {uintptr_t component{},component_vtable{};if(Read(item+0x50,&component,8) && component && Read(component,&component_vtable,8) && component_vtable==image+0x294FF98){
                    candidate.parameter_animations=component+8;
                    // Witness (light-observe.flag; its final review's L1, L2): each item whose owner the component gives, once, by model name,
                    // with its table's rows; the main walks only (not the rays'), 64 items at most.
                    static const bool witness=ModuleFlag(L"light-observe.flag");
                    if(witness && !native_rt_gates){static std::mutex component_lock;static std::set<uintptr_t> component_seen;bool fresh{};
                        {const std::lock_guard hold(component_lock);fresh=component_seen.size()<64 && component_seen.insert(item).second;}
                        if(fresh){uintptr_t table{},begin{},end{};if(Read(component+0x10,&table,8) && table){Read(table,&begin,8);Read(table+8,&end,8);}
                            std::string name;for(const auto ch:NativeModelDataName(candidate.model))if(name.size()<60)name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                            char row[240]{};sprintf_s(row,"{\"event\":\"native_material_param_component\",\"model\":\"%s\",\"rows\":%lld,\"offscreen\":%s}\n",
                                name.c_str(),static_cast<long long>(end>begin?(end-begin)/0x90:0),native_offscreen_preparing?"true":"false");Log(row);}}}}
                const auto instance=pointer(item+0x710);if(!instance)continue;
                candidate.pose=instance+0xA0;candidate.region_box=instance+0x80;candidate.sort_rule=1;   // I-0113 tranche 15: D3DA20's key, the box's depth extent alone
                candidate.fade_owner=entity;candidate.fade_layout={0x120,0x1C0,0x130,0x1F0,-1};
                candidate.velocity=(flags&2)!=0;   // 1186: D3DA20 copies the disp entity's +0x388 bit 1 to the draw's velocity byte
                candidate.fade_prepass=(flags&4)!=0;   // I-0113 tranche 10d: and bit 2 to its byte +0x10 (57A20's fading depth pre-pass)
                // 1169: a menu character part (offscreen view) carries an instance decal provider (blood) whose
                // resource contract is not owned: the same neutral decal as the world character's ASM parts.
                candidate.neutral_instance_decal=native_offscreen_preparing;
                // The item's FaceGen texture owner (FrpgFgTextureIns [item+770], MENUS-MAP 26) for the FG_face_gen slot, its draw colour
                // and its FaceGen vertex patch. I-0140 (the player's white face in cutscenes, I-0139 tranche 3): D3DA20 takes [item+770] (+0x30 once +0x91), copies
                // item+0x780..0x78C to the group's +0x40 and passes [item+758] for every item it draws, not for the menu views only:
                // a Remo cutscene draws its characters as such items (native_face_unbound: FG_A_0100 under 2951FF8 without an owner,
                // i140-e1-*), so their face, colours and FaceGen shape are taken in every view.
                candidate.draw_colour=item+0x780;if(!Read(item+0x770,&candidate.face_owner,8)){NoteNativeSkip("item_pointer_unreadable",item);continue;}
                // 1177: D3DA20 passes [item+758] as descriptor +28: the part's SprjFgModelVertexStreamPatch (51B890, vtable 27A01F8;
                // measured: [item+758] is the D3F7C0 receiver of this part's model), whose morphed positions replace stream 0 (FaceGen).
                {uintptr_t patch{};if(Read(item+0x758,&patch,8) && patch)candidate.stream_patch=patch;}
                // 1178 witness: D3DA20's descriptor +0xA0 texture choice for this part: [item+58]+358/+360 when item+6F2 (+6F3 picks),
                // FrpgFgTextureIns [item+770]+30 once +91 (hair parts sample the FLVER "hairoff" placeholder natively).
                if(native_offscreen_preparing){static LONG64 choice_rows{};if(const auto n=InterlockedIncrement64(&choice_rows);n<=10 || (n%40==0 && n<=1600)){BYTE use{},pick{},ready{};uintptr_t first{},second{},fg{},fg_texture{};
                    Read(item+0x6F2,&use,1);Read(item+0x6F3,&pick,1);Read(candidate.model+0x358,&first,8);Read(candidate.model+0x360,&second,8);
                    if(Read(item+0x770,&fg,8) && fg){Read(fg+0x91,&ready,1);Read(fg+0x30,&fg_texture,8);}
                    const auto name=[&](uintptr_t texture)->std::string{if(!texture)return "";const auto r=textures.Acquire(texture);
                        return r && r->content?r->content->name.substr(0,60):"(unowned)";};
                    char row[512]{};sprintf_s(row,"{\"event\":\"native_menu_part_texture_choice\",\"use\":%u,\"pick\":%u,\"first\":\"%s\",\"second\":\"%s\",\"facegen\":%s,\"facegen_ready\":%u,\"facegen_texture\":\"%s\"}\n",
                        use,pick,name(first).c_str(),name(second).c_str(),fg?"true":"false",ready,name(fg_texture).c_str());Log(row);}}
                BYTE no_geometry{};if(!Read(item+0x71A,&no_geometry,1)){NoteNativeSkip("item_policy_unreadable",item);continue;}   // I-0101
                candidate.geometry_suppressed=no_geometry!=0;
                // 1185 (MENUS-MAP 66): a SprjModel's cloth instance, copied to its item at +0x750 (C4C490).
                candidate.cloth_set=NativeClothSetOf(item+0x750,candidate.model,2,&candidate.cloth_instance);
                const auto transforms=pointer(item+0x6F8);if(!transforms)continue;
                const auto matrices=pointer(transforms+0x60);if(!matrices)continue;
                if(!Read(matrices,candidate.placement.data(),sizeof(candidate.placement))){NoteNativeSkip("item_placement_unreadable",item);continue;}   // I-0101
                if(visitor->rays_also)visitor->rays_also->push_back({candidate,0,0,0});   // I-0140 t31: a Player menu view's rays
                if(visitor->deferred){visitor->deferred->push_back({candidate,0,0,0});continue;}   // I-0071 point 3: admitted after the walk
                {auto& scene=*visitor->result;const auto models_before=scene.models.size();
                AdmitNativeModelCandidate(visitor,candidate);
                // 1184 witness: skinned map-object items (pose with more than one row) tallied under pseudo part 1001.
                uint32_t pose_rows{};if(!native_offscreen_preparing && scene.models.size()>models_before && Read(candidate.pose+0x70,&pose_rows,4) && pose_rows>1)
                    scene.models.back().asm_part=1001;}
            }
            return;
        }
        // 1056 witness (characters): SprjAsmModelDrawEntity (2947DB8) emit D00200 ->
        // D02A30 walks its parts: count +AF8, descriptors +B00 (stride 50: model +10,
        // flags +38), part objects +B10 (stride 108, emit vtable+50). Read only.
        if(entity_table==image+0x2947DB8 && !native_rt_gates){static LONG64 asm_witness{};const auto n=InterlockedIncrement64(&asm_witness);
            if(n<=3 || (n%300==0 && n<=3000)){BYTE enabled{};uint32_t category{},parts{};uintptr_t descriptors{},objects{};
                Read(entity+0xBB0,&enabled,1);Read(entity+0x70,&category,4);Read(entity+0xAF8,&parts,4);Read(entity+0xB00,&descriptors,8);Read(entity+0xB10,&objects,8);
                std::string rows;
                for(uint32_t i=0;i<parts && i<12 && descriptors && objects;++i){uintptr_t model{},object_table{},emit{};BYTE flags{};
                    Read(descriptors+0x50ull*i+0x10,&model,8);Read(descriptors+0x50ull*i+0x38,&flags,1);
                    if(Read(objects+0x108ull*i,&object_table,8) && object_table)Read(object_table+0x50,&emit,8);
                    char part[160]{};sprintf_s(part,"%s{\"model\":%s,\"flags\":%u,\"object_table\":%llu,\"emit50\":%llu}",rows.empty()?"":",",model?"true":"false",flags,
                        static_cast<unsigned long long>(object_table?object_table-image:0),static_cast<unsigned long long>(emit?emit-image:0));rows+=part;}
                std::string row="{\"event\":\"native_asm_entity\",\"enabled\":"+std::to_string(enabled)+",\"category\":"+std::to_string(category)+
                    ",\"parts\":"+std::to_string(parts)+",\"items\":["+rows+"]}\n";Log(row.c_str());}}
        // 1057 characters: SprjAsmModelDrawEntity (2947DB8). D00200 gates on +BB0 bit0,
        // D03430 and +70 & the world mask, uses the SprjModelDrawEntity fade layout
        // (+160/+200, group +240, suppression +A5D), then D02A30 emits each part i
        // (descriptors +B00 stride 50: binding +10, flags +38 with bits 0-3 set and
        // bit 4 clear; part objects +B10 stride 108) through CFDE70, which builds the
        // D079C0 descriptor with: model [binding]+8, matrix container entity+A60
        // (+18 when +BB0 bit3) -> +10, stream source part+28 (+50 patch), parameter
        // animations = descriptor+20 = D01C30(entity,i) = [entity+B18]+10*i ([part+40]
        // goes to descriptor+28, the DD5CF0 slot; 1066 correction), storage part+50
        // (59040's last argument). Culling by
        // bounds is not applied to these few entities (explicit initial policy).
        if(entity_table==image+0x2947DB8){
            uint32_t state{},category{},parts{};uintptr_t descriptors{},objects{};
            if(!Read(entity+0xBB0,&state,4) || !Read(entity+0x70,&category,4) || !Read(entity+0xAF8,&parts,4) ||
               !Read(entity+0xB00,&descriptors,8) || !Read(entity+0xB10,&objects,8)){NoteNativeSkip("asm_unreadable",entity);return;}   // I-0101
            if(!(state&1)){popin_gate(0x22);return;}
            if(!(category&visitor->world_mask)){popin_gate(0x24);return;}
            if(!descriptors || !objects || parts>256)return;
            if(!reinterpret_cast<BYTE(__fastcall*)(uintptr_t)>(image+0xD03430)(entity)){popin_gate(0x23);return;}
            popin_gate(0x20);
            const auto matrices=pointer(entity+0xA60+((state>>3)&1)*0x18+0x10);if(!matrices)return;
            std::array<float,12> placement{};
            if(!Read(matrices,placement.data(),sizeof(placement))){NoteNativeSkip("asm_placement_unreadable",entity);return;}   // I-0101
            for(uint32_t i=0;i<parts;++i){
                const uintptr_t descriptor=descriptors+0x50ull*i,part=objects+0x108ull*i;
                BYTE flags{};if(!Read(descriptor+0x38,&flags,1)){NoteNativeSkip("asm_part_unreadable",descriptor);continue;}   // I-0101
                if((flags&0x10) || !(flags&0xF))continue;
                const auto binding=pointer(descriptor+0x10);if(!binding)continue;
                NativeSelectionCandidate candidate;candidate.entity=part;candidate.model=pointer(binding+8);if(!candidate.model)continue;
                candidate.group=entity+0x240;candidate.animation=entity+0x3D0;candidate.source_field=part+0x28;candidate.capability_source=descriptor;
                {const auto owner=pointer(part+0x18);if(owner && pointer(owner)==image+0x2947DB8){candidate.region_box=owner+0xAD0;candidate.region_box_w=true;
                    candidate.sort_rule=2;candidate.sort_extra=owner+0xAF0;}}   // I-0113 tranche 14; tranche 15: CFDE70's key
                candidate.pose=part+0x50;if(!Read(part+0x48,&candidate.face_owner,8)){NoteNativeSkip("asm_part_unreadable",part);continue;}   // I-0101: not "absent"
                // CFDE70 draws each part with a stack copy of the group whose +0x40 (state +0x280, FC_SkinMatchColor) is
                // E+0xBD0+0x10*[descriptor+0x48] (the appearance colours D009C0 writes; MENUS-MAP 60); the shared group
                // E+0x280 itself is never written by the original.
                {BYTE colour{};if(!Read(descriptor+0x48,&colour,1)){NoteNativeSkip("asm_part_unreadable",descriptor);continue;}   // I-0101
                    if(colour<8)candidate.draw_colour=entity+0xBD0+0x10ull*colour;}   // D009C0 writes c < 8 only
                // P4: CFDE70's phantom row for desc+0x44 > 0 (PhantomParam lookup E35EF0: {id, row}; fade +0x24, kind +0x2C).
                {int16_t id{};if(Read(descriptor+0x44,&id,2) && id>0){
                    alignas(16) std::array<uint8_t,16> handle{};
                    reinterpret_cast<void(__fastcall*)(void*,int32_t)>(image+0xE35EF0)(handle.data(),id);
                    uintptr_t row{};std::memcpy(&row,handle.data()+8,8);float fade{};BYTE kind{};
                    if(row && Read(row+0x24,&fade,4) && Read(row+0x2C,&kind,1) && std::isfinite(fade)){
                        candidate.phantom=true;candidate.phantom_fade=fade;candidate.phantom_kind=kind<4?kind:4;
                        // The same series as reference_phantom_state (light-observe.flag): the group's mulCol with .w times the row fade.
                        static const bool phantoms=ModuleFlag(L"light-observe.flag");
                        if(phantoms){std::array<float,4> colour{};Read(candidate.group+0xD0,colour.data(),16);colour[3]*=fade;
                            static std::mutex lock;static std::unordered_map<uintptr_t,int> last;static int rows{};bool fresh{};const int alpha=static_cast<int>(std::lround(colour[3]*1000.f));
                            {const std::lock_guard hold(lock);if(rows<300){const auto found=last.find(candidate.model);if(found==last.end() || found->second!=alpha){last[candidate.model]=alpha;++rows;fresh=true;}}}
                            if(fresh){char text[320]{};sprintf_s(text,"{\"event\":\"native_phantom_state\",\"model\":\"%.60s\",\"kind\":%u,\"fade\":%g,\"mul_col\":[%g,%g,%g,%g]}\n",
                                NativeModelDataName(candidate.model).c_str(),kind,fade,colour[0],colour[1],colour[2],colour[3]);Log(text);}}}}}
                // 1185 (MENUS-MAP 66): the part's cloth instance is descriptor+0x18 (C37B70 -> C487D0; INFERRED).
                candidate.cloth_set=NativeClothSetOf(descriptor+0x18,candidate.model,1,&candidate.cloth_instance);
                {uintptr_t owners{};if(!Read(entity+0xB18,&owners,8)){NoteNativeSkip("asm_part_unreadable",entity);continue;}   // I-0101
                    if(owners)candidate.parameter_animations=owners+0x10ull*i;}
                candidate.fade_owner=entity;candidate.placement=placement;candidate.neutral_instance_decal=true;
                {BYTE moved{};candidate.velocity=(state&0x40) && (state&0x80) && Read(entity+0x184,&moved,1) && moved;}   // 1186: CFDE70
                candidate.fade_prepass=(state&0x100) && (flags&0x20);   // I-0113 tranche 10d: CFDE70's byte +0x10 (the part's flags +0x38 bit 0x20)
                if(!native_rt_gates)native_asm_parts.fetch_add(1,std::memory_order_relaxed);
                if(visitor->rays_also)visitor->rays_also->push_back({candidate,1,0,static_cast<int32_t>(i)});   // I-0140 t31
                if(visitor->deferred){visitor->deferred->push_back({candidate,1,0,static_cast<int32_t>(i)});continue;}   // I-0071 point 3
                auto& scene=*visitor->result;const auto models_before=scene.models.size();
                const std::array<uint32_t,6> before{scene.zero_fade,scene.missing_bindings,scene.instance_streams,scene.missing_geometry,scene.no_meshes,scene.nonfinite};
                AdmitNativeModelCandidate(visitor,candidate);
                if(scene.models.size()>models_before)scene.models.back().asm_part=static_cast<int32_t>(i);
                else{const std::array<uint32_t,6> after{scene.zero_fade,scene.missing_bindings,scene.instance_streams,scene.missing_geometry,scene.no_meshes,scene.nonfinite};
                    uint8_t code=0;for(uint8_t k=0;k<5;++k)if(after[k]!=before[k]){code=static_cast<uint8_t>(k+1);break;}
                    if(!code && after[5]!=before[5])code=7;   // final review L5: « nonfinite », as the deferred path names it
                    scene.asm_rejected.push_back({static_cast<int32_t>(i),code});}
            }
            return;
        }
        if(entity_table!=image+0x2948EF8 && native_rt_gates)return;   // I-0117 T1 step review L3: the rays' walk leaves the main view's tallies alone
        if(entity_table!=image+0x2948EF8)SampleNativeUnsupportedSelection(entity,entity_table);
        if(entity_table!=image+0x2948EF8){
            ++result.unsupported;static LONG64 samples{};
            if(InterlockedIncrement64(&samples)<=8){char row[256]{};sprintf_s(row,
                "{\"event\":\"native_selected_object_unsupported\",\"vtable_rva\":%llu,\"emit_rva\":%llu,\"bounds_rva\":%llu}\n",
                static_cast<unsigned long long>(entity_table-image),
                static_cast<unsigned long long>(pointer(entity_table+8)-image),
                static_cast<unsigned long long>(pointer(entity_table+0x18)-image));Log(row);}
            return;
        }
        if(pointer(pointer(entity)+0x98)!=image+0xD07E10){NoteNativeSkip("entity_bounds_identity",entity);return;}   // I-0101
        // D07E10's actual bounds getter is entity+B80. No original occlusion
        // history exists: native depth-occlusion is explicitly unavailable.
        // Fable review (the Crucifixion Woods cross, o000499 at m33: Reference2175 emits it in the main pass with bounds all zero,
        // the native walk never visited it): an entity whose bounds E+0xB80 were never written (all eight words zero; MENUS-MAP 62
        // section 2, "the constructor does not initialise B80") is not culled by them, as the original's emit shows. The shadow
        // gate keeps its box test (a zero box casts in no cascade).
        bool bounds_unset=false;alignas(16) float box[8]{};const bool box_read=Read(entity+0xB80,box,sizeof box);
        if(box_read){bounds_unset=true;for(const auto v:box)if(v!=0.f){bounds_unset=false;break;}}
        if(!inside && !bounds_unset && !classify(culler,entity+0xB80)){popin_gate(0x11);return;}
        if(margin_kept(inside,bounds_unset))popin_gate(0x16);
        BYTE enabled{};uint32_t category{};
        if(!Read(entity+0xC3C,&enabled,1) || !Read(entity+0x70,&category,4)){NoteNativeSkip("entity_policy_unreadable",entity);return;}   // I-0101
        if(!(enabled&1)){popin_gate(0x12);return;}
        if(!(category&visitor->world_mask)){popin_gate(0x14);return;}
        // D03430 = intersection of eight CPU visibility words != the genuine
        // neutral vector. Call the actual CPU implementation, not a GPU emitter.
        if(!reinterpret_cast<BYTE(__fastcall*)(uintptr_t)>(image+0xD03430)(entity)){popin_gate(0x13);return;}
        popin_gate(0x10);
        const auto binding=pointer(entity+0xE8);if(!binding)return;
        NativeSelectionCandidate candidate;candidate.entity=entity;candidate.model=pointer(binding+8);if(!candidate.model)return;
        candidate.group=entity+0x240;candidate.animation=entity+0x3D0;candidate.source_field=entity+0xA60;
        candidate.instance_field=entity+0xA48;candidate.pose=entity+0xA80;candidate.parameter_animations=entity+0xA40;
        candidate.fade_owner=entity;candidate.region_box=entity+0xB80;candidate.sort_rule=1;candidate.sort_extra=entity+0xBA0;   // tranche 15: D079C0's key
        {BYTE keep[2]{},moved{};candidate.velocity=Read(entity+0xBA5,keep,2) && keep[0] && keep[1] && Read(entity+0x184,&moved,1) && moved;}   // 1186: D079C0
        {BYTE prepass{};candidate.fade_prepass=Read(entity+0xBA7,&prepass,1) && prepass;}   // I-0113 tranche 10d: D079C0's byte +0x10
        // 1185 (MENUS-MAP 66): E+0xA70 is the cloth instance D079C0 reads (DD5D10/DD5CF0).
        candidate.cloth_set=NativeClothSetOf(entity+0xA70,candidate.model,0,&candidate.cloth_instance);
        // D079C0 -> 7E2300 chooses one of the real two 0x18 matrix containers.
        const auto source=pointer(entity+0xB08+((enabled>>4)&1)*0x18+0x10);
        if(!Read(source,candidate.placement.data(),sizeof(candidate.placement))){NoteNativeSkip("placement_unreadable",entity);return;}   // I-0101
        if(visitor->rays_also)visitor->rays_also->push_back({candidate,2,0,0});   // I-0140 t31
        if(visitor->deferred){visitor->deferred->push_back({candidate,2,0,0});return;}   // I-0071 point 3
        auto& scene=*visitor->result;const auto models_before=scene.models.size();
        AdmitNativeModelCandidate(visitor,candidate);
        // 1184 witness: SprjModelDrawEntity models (characters and skinned map objects) share the asm part outcome tally
        // under the pseudo part 1000, so their pass gates and draw failures are logged by family.
        if(scene.models.size()>models_before)scene.models.back().asm_part=1000;
    }catch(...){if(control->stop)return;if(native_rt_gates){rt_skip("rt_walk_entity_failed");return;}Refuse("native_scene_selection_admission_failed");}
}
// I-0071 point 3: the admission pool is ds3native::AdmissionPool (renderer/work_pool.h, shared with the recording pool since I-0074).
// After the main walk: its candidates admitted on the pool in chunks taken by ticket, each worker keeping its admitted models
// with their candidate's index; then merged in walk order into the scene (the same order as one thread would give), with the
// part tags of the entity filter. The scene's lazily built members are built first; fewer than 64 candidates are admitted on
// this thread alone.
void AdmitNativeDeferredCandidates(NativeSelectionVisitor& visitor,std::vector<NativeDeferredCandidate>& deferred) {
    auto& result=*visitor.result;visitor.deferred=nullptr;
    if(deferred.empty())return;
    if(!result.scene_word_ranges)result.scene_word_ranges=NativeShaderWordRanges(result.camera_constants.bytes,result.camera_constants.written);
    EnsureNativeSystemTextures(result);
    auto& pool=AdmissionPool();
    const size_t workers=deferred.size()<64?1:pool.Workers();
    // I-0077: one slot per candidate, kept by the collecting thread from one collect to the next (emptied after the merge).
    // (a reference: inside the workers' job a thread_local name would be each worker's own empty list)
    thread_local std::vector<std::optional<NativeSelectedModel>> kept_slots;auto& slots=kept_slots;if(slots.size()<deferred.size())slots.resize(deferred.size()+deferred.size()/8);
    std::atomic<size_t> ticket{};const size_t count=deferred.size();
    const bool offscreen=native_offscreen_preparing;const NativeViewFlags view;
    // I-0097 tranche 2 (crossing-observe.flag, the main walk only): per worker its admissions, their time and the fresh ones'
    // time; a row for a collect whose admission took 3 ms or more (the area crossing's).
    const bool observe=NativeCrossingObserve() && !offscreen;
    struct WorkerTimes {int64_t busy{},fresh_ticks{},reuse_ticks{},longest{};uint32_t taken{},fresh{};size_t longest_index{};std::array<int64_t,kprofile::Count> split{};};
    const auto times_owner=observe?std::make_unique<std::array<WorkerTimes,16>>():nullptr;   // final review: none unless observed
    LARGE_INTEGER started{};if(observe)QueryPerformanceCounter(&started);
    // I-0100 T0 (popin-observe.flag, the main walk only): each fresh admission's time (its record built), for the rows of the
    // models first met (the camera turn's first admissions, I-0097 tranche 5 item 10), popin_first, below.
    const bool popin=PopinObserve() && !offscreen;
    std::vector<float> fresh_times;if(popin)fresh_times.assign(count,-1.f);
    const int64_t popin_frequency=popin?[]{LARGE_INTEGER f{};QueryPerformanceFrequency(&f);return f.QuadPart;}():1;
    const std::function<void(size_t)> job=[&](size_t worker){
        if(worker>=workers)return;
        const NativeViewFlags::Apply view_flags(view);   // I-0077: the collecting thread's view
        native_admission_sink=slots.data();
        struct Clear {~Clear(){native_admission_sink=nullptr;}} clear;   // also when an admission throws
        // I-0078: chunks of 16 candidates while many remain, then smaller ones (the last chunks no longer leave one thread
        // admitting 16 models while the others wait).
        for(;;){const size_t seen=ticket.load(std::memory_order_relaxed);if(seen>=count)break;
            const size_t take=std::clamp<size_t>((count-seen)/(4*workers),1,16);
            const size_t begin=ticket.fetch_add(take,std::memory_order_relaxed);if(begin>=count)break;
            for(size_t i=begin;i<begin+take && i<count;++i){LARGE_INTEGER a{};const uint64_t fresh_before=native_fresh_admissions;std::array<int64_t,kprofile::Count> phases;   // set under observe only (final review)
                if(observe){kprofile::Flush();QueryPerformanceCounter(&a);phases=kprofile::thread_ticks;}
                LARGE_INTEGER popin_start{};if(popin)QueryPerformanceCounter(&popin_start);
                native_admission_index=i;AdmitNativeModelCandidate(&visitor,deferred[i].candidate);deferred[i].code=native_admission_code;
                if(popin && native_fresh_admissions!=fresh_before){LARGE_INTEGER end{};QueryPerformanceCounter(&end);
                    fresh_times[i]=float(1000.0*double(end.QuadPart-popin_start.QuadPart)/double(popin_frequency));}
                if(observe && worker<16){kprofile::Flush();LARGE_INTEGER b{};QueryPerformanceCounter(&b);auto& t=(*times_owner)[worker];const int64_t d=b.QuadPart-a.QuadPart;
                    ++t.taken;t.busy+=d;if(d>t.longest){t.longest=d;t.longest_index=i;for(size_t k=0;k<phases.size();++k)t.split[k]=kprofile::thread_ticks[k]-phases[k];}
                    if(native_fresh_admissions!=fresh_before){++t.fresh;t.fresh_ticks+=d;}else t.reuse_ticks+=d;}
                // 1184 witness tags: skinned map-object items (a pose of more than one row) 1001, SprjModelDrawEntity models 1000,
                // the assembled characters' parts their index (set here, on the worker, I-0077).
                if(auto& model=slots[i]){const auto& item=deferred[i];
                    if(item.kind==0){uint32_t pose_rows{};if(!offscreen && Read(item.candidate.pose+0x70,&pose_rows,4) && pose_rows>1)model->asm_part=1001;}
                    else if(item.kind<3)model->asm_part=item.kind==1?item.part:1000;}}}
        // I-0078: each worker's admission marks join the main walk's sections (the other walks' are dropped on the pool workers;
        // review L2: the collecting thread keeps its pending ones for the main collect's flush).
        if(NativeAdmissionMarksOn()){if(!native_offscreen_preparing)FlushNativeAdmissionMarks();
            else if(worker)for(size_t i=0;i<12;++i){InterlockedExchange64(&admission_marks.ticks[i],0);InterlockedExchange64(&admission_marks.counts[i],0);}}};
    struct Empty {std::vector<std::optional<NativeSelectedModel>>& slots;size_t count;~Empty(){for(size_t i=0;i<count;++i)slots[i].reset();}} empty{slots,deferred.size()};   // also on a throw
    if(workers==1)job(0);else pool.Run(job);
    if(observe){LARGE_INTEGER ended{},frequency{};QueryPerformanceCounter(&ended);QueryPerformanceFrequency(&frequency);
        const auto ms=[&](int64_t ticks){return 1000.0*double(ticks)/double(frequency.QuadPart);};const double wall=ms(ended.QuadPart-started.QuadPart);
        if(wall>=3.0){std::string busy;uint32_t fresh{};double fresh_ms{},reuse_ms{},longest{};size_t slowest{};
            const auto& times=*times_owner;for(size_t w=0;w<workers && w<times.size();++w){const auto& t=times[w];char cell[48]{};sprintf_s(cell,"%s[%u,%.2f]",w?",":"",t.taken,ms(t.busy));busy+=cell;
                fresh+=t.fresh;fresh_ms+=ms(t.fresh_ticks);reuse_ms+=ms(t.reuse_ticks);if(ms(t.longest)>longest){longest=ms(t.longest);slowest=w;}}
            // the slowest admission: its model's name, its kind (0 item, 1 assembled part, 2 draw entity, 3 model particle) and its time per collect phase
            const auto& s=times[slowest];std::string split;for(size_t k=0;k<kprofile::CollectCount;++k){char cell[24]{};sprintf_s(cell,"%s%.2f",k?",":"",ms(s.split[k]));split+=cell;}
            std::string name=NativeModelDataName(deferred[s.longest_index].candidate.model);for(auto& ch:name)if(ch=='"' || ch=='\\' || ch<32 || ch>126)ch='?';
            char head[640]{};sprintf_s(head,"{\"event\":\"native_admission_slow\",\"serial\":%llu,\"candidates\":%zu,\"workers\":%zu,\"wall_ms\":%.2f,\"fresh\":%u,\"fresh_ms\":%.2f,\"reuse_ms\":%.2f,\"longest_ms\":%.2f,\"slowest\":{\"model\":\"%.60s\",\"kind\":%u,\"split_ms\":[%s],\"profiled\":%s},\"workers_taken_ms\":[",
                static_cast<unsigned long long>(result.serial),count,workers,wall,fresh,fresh_ms,reuse_ms,longest,name.c_str(),unsigned(deferred[s.longest_index].kind),split.c_str(),kprofile::enabled.load(std::memory_order_relaxed)?"true":"false");
            Log((std::string(head)+busy+"]}\n").c_str());}}
    // Only a model's first admission in the run is a row (the i100-t0-settlement-n1 run: c1200, c1240, FG_A_0100 and some o392*
    // build their record afresh at every admission, 107,121 rows); the others are summed per collect.
    if(popin){static std::mutex lock;static std::unordered_set<uint64_t> seen;double again_ms{};uint32_t again{};
        const std::lock_guard hold(lock);
        for(size_t i=0;i<count;++i)if(fresh_times[i]>=0.f){const auto& c=deferred[i].candidate;
            if(!seen.insert(uint64_t(c.entity)^(uint64_t(c.model)<<1)).second){again_ms+=fresh_times[i];++again;continue;}
            char row[320]{};sprintf_s(row,"{\"event\":\"popin_first\",\"serial\":%llu,\"entity\":\"%llx\",\"model\":\"%s\",\"kind\":%u,\"admitted\":%d,\"ms\":%.3f,\"d\":%.2f}\n",
                static_cast<unsigned long long>(result.serial),static_cast<unsigned long long>(c.entity),PopinName(c.model).c_str(),unsigned(deferred[i].kind),slots[i]?1:0,fresh_times[i],
                PopinPlaceAt(c.placement.data(),0).d);Log(row);}
        if(again_ms>=1.0){char row[160]{};sprintf_s(row,"{\"event\":\"popin_rebuilt\",\"serial\":%llu,\"count\":%u,\"ms\":%.3f}\n",static_cast<unsigned long long>(result.serial),again,again_ms);Log(row);}}
    // The models in candidate order: their places counted first, then (I-0077, i74-native-61: the serial moves were a third of
    // this function on the engine thread) each moved on the pool, by chunks of candidates, into its place: I-0091 (ER: an entity's
    // record written once where it stays), places nothing initialised before (the moves cannot throw).
    std::vector<size_t> place(deferred.size());size_t next_place=0;
    for(size_t i=0;i<deferred.size();++i){
        if(slots[i])place[i]=next_place++;
        else if(deferred[i].kind==1)result.asm_rejected.push_back({deferred[i].part,deferred[i].code});   // 1068: the first refusing cause
    }
    NativeSelectedModel* const places=result.models.Uninitialized(next_place);
    std::atomic<size_t> next_move{};constexpr size_t moves_per_chunk=64;
    const std::function<void(size_t)> move=[&](size_t){
        for(size_t begin;(begin=next_move.fetch_add(moves_per_chunk,std::memory_order_relaxed))<deferred.size();)
            for(size_t i=begin;i<begin+moves_per_chunk && i<deferred.size();++i)
                if(slots[i])::new(static_cast<void*>(places+place[i])) NativeSelectedModel(std::move(*slots[i]));};
    if(workers==1)move(0);else pool.Run(move);
    result.models.Commit(next_place);
}
}  // namespace
