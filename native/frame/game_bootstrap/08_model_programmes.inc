// Model programmes and mesh bindings: composed groups, block and texture plans, programme analysis, the selection
// candidate, mesh blocks and textures (planned and per-mesh), lightmaps and the persistent model records. Part of
// game_bootstrap.cpp's single translation unit (the order is there): it relies on every earlier part (07_scene_lighting.inc
// before it).
#pragma once
#include "07_scene_lighting.inc"
namespace {
std::shared_ptr<const NativeEffect> ComposeNativeShaderGroup(const std::wstring& group,size_t& candidates) {
    static const bool lighting_witnesses=ModuleFlag(L"light-observe.flag");   // I-0120 C8: the two one-time scans of the first composition only with light-observe.flag
    if(lighting_witnesses)WitnessLightingProgrammes();
    {static LONG64 effect_scan{};if(lighting_witnesses && InterlockedIncrement64(&effect_scan)==1){std::string names;size_t matched{};
        {const std::lock_guard hold(effect_lock);for(const auto& [key,effect]:effects){(void)key;for(const auto& program:effect->program_names)
            if(program.name.find(L"GXLight")!=std::wstring::npos || program.name.find(L"Composite")!=std::wstring::npos || program.name.find(L"DepthScale")!=std::wstring::npos){
                ++matched;if(names.size()<2000){names+=names.empty()?"":",";for(const auto ch:program.name)names.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');}}}}
        std::string row="{\"event\":\"native_lighting_effect_programmes\",\"matched\":"+std::to_string(matched)+",\"names\":\""+names+"\"}\n";Log(row.c_str());}}
    // I-0120 C5: a group that composed nothing (no vertex and pixel programme among the loaded ones) is answered empty again,
    // without the two repository walks, while no shader was admitted or retired since (shader_registry_changes) and for at most
    // one second (a repository node the engine links after its shader's admission, Native2202, is then found).
    struct Failed {uint64_t changes{};ULONGLONG at{};};
    static std::unordered_map<std::wstring,Failed> failed;   // under composed_effect_lock
    const auto changes=shader_registry_changes.load(std::memory_order_acquire);
    {const std::lock_guard hold(composed_effect_lock);const auto found=composed_effects.find(group);
        if(found!=composed_effects.end())return found->second;
        if(const auto miss=failed.find(group);miss!=failed.end() && miss->second.changes==changes && GetTickCount64()-miss->second.at<1000)return {};}
    // 1186: and the Velo programmes "<group>_Velo..." (shader pass 1) of the same loose group.
    const auto prefix=group+L"_Gbuf",velocity_prefix=group+L"_Velo",depth_prefix=group+L"_Dpt";   // D-0180 C2: and Dpt/DptA
    const auto water_prefix=group+L"_WatDpt";   // WATER-TESS-WAVE1-READY WT-1: the water rows' WatDpt (GXFlver_WaterBumpTri, a loose group)
    // m50 sky (Native1862: the eclipse piece m090002, GXFlver_Col with a forward-only material, refused at the 0x29 row for want
    // of a Fwd recipe): and the forward programmes "<group>_Fwd..." (shader pass 0: Fwd, Fwd_[A], FwdSdw, FwdSSdw) of the group.
    const auto forward_prefix=group+L"_Fwd";
    auto owned=std::make_shared<NativeEffect>();
    struct Stage { uintptr_t repository;uint32_t kind;const wchar_t* extension;ds3content::ShaderStage stage; };
    // W7 TS-2b: with native-tess.flag, the hull (4792B08, kind 2) and domain (4792838, kind 1) repositories too (D1E570,
    // D1E3D0: LEGACY-ROOT-INVENTORY-903), for the `<group>_GBuf_PN` hull and domain of a composed group.
    std::vector<Stage> stages{{0x4792BF8,4,L".ppo",ds3content::ShaderStage::Pixel},{0x4792D58,5,L".vpo",ds3content::ShaderStage::Vertex}};
    if(NativeTessellationEnabled()){stages.push_back({0x4792B08,2,L".hpo",ds3content::ShaderStage::Hull});stages.push_back({0x4792838,1,L".dpo",ds3content::ShaderStage::Domain});}
    for(const Stage& stage:stages){
        uintptr_t repository{},buckets{};uint32_t count{};
        if(!Read(image+stage.repository,&repository,8) || !repository || !Read(repository+0x60+0x1C,&count,4) ||
           !count || count>(1u<<20) || !Read(repository+0x60+0x20,&buckets,8) || !buckets){
            static LONG64 unavailable{};
            if(InterlockedIncrement64(&unavailable)<=2){char row[160]{};sprintf_s(row,"{\"event\":\"native_loose_programme_repository\",\"kind\":%u,\"available\":false,\"present\":%s}\n",
                stage.kind,repository?"true":"false");Log(row);}
            continue;}
        size_t visited{};
        for(uint32_t bucket=0;bucket<count && visited<200000;++bucket){
            uintptr_t node{};if(!Read(buckets+bucket*8ull,&node,8))break;
            for(;node && visited<200000;++visited){
                std::wstring name;
                if(ReadEngineWide(node+0x10,name) && ((name.size()>=prefix.size() && // the plain key equals the prefix (Native1030)
                   _wcsnicmp(name.c_str(),prefix.c_str(),prefix.size())==0) ||
                   (name.size()>=velocity_prefix.size() && _wcsnicmp(name.c_str(),velocity_prefix.c_str(),velocity_prefix.size())==0) ||
                   (name.size()>=depth_prefix.size() && _wcsnicmp(name.c_str(),depth_prefix.c_str(),depth_prefix.size())==0) ||
                   (name.size()>=water_prefix.size() && _wcsnicmp(name.c_str(),water_prefix.c_str(),water_prefix.size())==0) ||
                   (name.size()>=forward_prefix.size() && _wcsnicmp(name.c_str(),forward_prefix.c_str(),forward_prefix.size())==0))){
                    ++candidates;
                    // Keys may carry the programme extension; normalize to one.
                    if(name.size()>4 && _wcsicmp(name.c_str()+name.size()-4,stage.extension)==0)name.resize(name.size()-4);
                    std::shared_ptr<const NativeShader> program;
                    {const std::lock_guard hold(shader_lock);const auto found=shaders.find(node+0x68);
                        constexpr UINT engine_to_dxbc[]{5,4,3,2,0,1};
                        if(found!=shaders.end() && D3D11_SHVER_GET_TYPE(found->second->version)==engine_to_dxbc[stage.kind])program=found->second;}
                    if(program){owned->programs.push_back({name+stage.extension,stage.kind,std::move(program)});
                        owned->program_names.push_back({owned->programs.back().name,stage.stage});}
                }
                if(!Read(node+0x50,&node,8))break;
            }
        }
        // 1025 witness: repository population and one key shape, once per stage.
        static LONG64 reported[6]{};
        if(InterlockedIncrement64(&reported[stage.kind])==1){
            std::string sample;
            for(uint32_t bucket=0;bucket<count && sample.empty();++bucket){uintptr_t node{};
                if(Read(buckets+bucket*8ull,&node,8) && node){std::wstring name;
                    if(ReadEngineWide(node+0x10,name))for(const auto ch:name)sample.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');}}
            char row[512]{};sprintf_s(row,"{\"event\":\"native_loose_programme_repository\",\"kind\":%u,\"buckets\":%u,\"records\":%zu,\"sample\":\"%.200s\"}\n",
                stage.kind,count,visited,sample.c_str());Log(row);
        }
    }
    const bool vertex=std::any_of(owned->program_names.begin(),owned->program_names.end(),[](const auto& p){return p.stage==ds3content::ShaderStage::Vertex;});
    const bool pixel=std::any_of(owned->program_names.begin(),owned->program_names.end(),[](const auto& p){return p.stage==ds3content::ShaderStage::Pixel;});
    if(!vertex || !pixel){const std::lock_guard hold(composed_effect_lock);if(failed.size()>=1024)failed.clear();
        failed.insert_or_assign(group,Failed{changes,GetTickCount64()});return {};}
    {const std::lock_guard hold(effect_lock);owned->generation=++effect_generation;}
    std::shared_ptr<const NativeEffect> result=std::move(owned);
    const std::lock_guard hold(composed_effect_lock);
    const auto inserted=composed_effects.emplace(group,result);if(inserted.second)BumpProgramRegistry();return inserted.first->second;
}
// Workstream K P1: the per-programme half of the collect's block families (AdmitNativeModelCandidate), computed once.
// Each value is what the per-mesh code computed from the same inputs before this split (equivalence by construction).
// I-0100 tranche 7: made from the interfaces, reads, effect and authored material alone, so shared by the layouts that have them.
std::shared_ptr<const NativeModelLayout::Programs::BlockPlans> PlanNativeModelBlocks(const NativeModelLayout::Programs& programs) {
    auto made=std::make_shared<NativeModelLayout::Programs::BlockPlans>();
    for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage){
        const auto& shader=programs.interfaces[recipe][stage];if(!shader)continue;
        const auto& reads=programs.ReadsOf(recipe,stage);auto& plan=(*made)[recipe][stage];
        plan.numeric=ds3content::PlanReadShaderRegister(*shader,reads,3);
        plan.scene=ds3content::PlanReadShaderRegister(*shader,reads,8,"cbSceneParam");
        if(!stage)plan.view=ds3content::PlanReadShaderRegister(*shader,reads,0,"cbMatrixWVP");
        plan.instance=ds3content::PlanReadShaderRegister(*shader,reads,4,"cbInstanceData");
        const auto instance=std::find_if(shader->constants.begin(),shader->constants.end(),
            [](const auto& candidate){return candidate.name=="cbInstanceData";});
        if(instance!=shader->constants.end() && instance->declared_bytes>ds3prod::instance_record_bytes && instance->declared_bytes<=65536)
            plan.instance_tail=instance->declared_bytes-ds3prod::instance_record_bytes;
        const auto declaration=std::find_if(shader->constants.begin(),shader->constants.end(),
            [](const auto& candidate){return candidate.name=="cbMtdParam";});
        if(declaration==shader->constants.end())continue;
        plan.mtd_declared=true;
        // 1112: a builtin variable no declaration writes comes from the material's MTD values by name (ProduceMtdParams).
        for(const auto& variable:declaration->variables){
            auto& entry=plan.mtd_variables.emplace_back();entry.offset=variable.byte_offset;entry.bytes=variable.bytes;
            entry.fallback=programs.authored && variable.bytes<=64 && variable.byte_offset+variable.bytes<=declaration->declared_bytes;
            if(!entry.fallback)continue;
            std::vector<ds3prod::ConstantVariable> one{{variable.name,variable.byte_offset,variable.bytes}};
            std::vector<uint8_t> fallback_bytes(declaration->declared_bytes),written(declaration->declared_bytes/4);
            ds3prod::ProduceMtdParams(programs.authored->parameters,one,fallback_bytes.data(),declaration->declared_bytes,written.data());
            for(uint32_t at=variable.byte_offset;at<variable.byte_offset+variable.bytes;at+=4)if(written[at/4]){
                uint32_t word{};std::memcpy(&word,fallback_bytes.data()+at,4);entry.words.push_back({at,word});}
        }
        // A composed group (loose programmes, no SPX declarations): cbMtdParam by RDEF variable name from the
        // material's MTD values, the old host's ProduceMtdParams (I-0063, produced).
        if(programs.effect->parameter_declarations.empty() && declaration->declared_bytes && declaration->declared_bytes<=65536 &&
           !(declaration->declared_bytes%4)){
            std::vector<ds3prod::ConstantVariable> mtd_layout;
            for(const auto& variable:declaration->variables)mtd_layout.push_back({variable.name,variable.byte_offset,variable.bytes});
            std::vector<uint8_t> bytes(declaration->declared_bytes),written(declaration->declared_bytes/4);
            ds3prod::ProduceMtdParams(programs.authored->parameters,mtd_layout,bytes.data(),declaration->declared_bytes,written.data());
            const auto mtd_ranges=NativeShaderWordRanges(bytes,written);
            plan.composed_numeric=std::make_shared<const ds3content::ShaderRegisterBlock>(
                ds3content::PrepareReadShaderRegister(*shader,reads,3,mtd_ranges,"cbMtdParam"));
        }
    }
    return made;
}
// Workstream K P4b (K-COLLECT-IMPLEMENTATION-SHEET 2.4): the engine's named texture registry [144792F58]+0x60, which D24490
// looks paths up in. Its mutators found statically are observed pass-through (integer arguments only, at most six) and
// bump the generation after they return: acquire by name D4F250, the three-or-more-names path D4E100 (reached by the
// D4F470 thunk), find-or-insert D4F690 (and its D4F770 wrapper), the map insert D4EE10, erase or reset D4F4B0, release
// D4F7A0. Not observed (second review): D4A120's find-or-insert through D4C200 -> D50AB0, and E4B1A0/E4B8C0, which delete
// and recreate the whole registry. A kept choice therefore also compares the registry root, keeps only first-candidate
// results (an insert cannot outrank them), re-reads the holder's vtable and texture words, and is dropped by a shadow
// mismatch; sheet gap (c) stays open.
std::atomic<uint64_t> texture_registry_generation{1};
// I-0120 C1: D24490 walks the registry's hash without a lock, while the admission workers look paths up beside the loader
// threads that insert and erase in it. The module's lookups never block on a writer (the final review's M1: D4E100 holds the
// engine lock 144793D28 across a whole TPF creation and its fragments' join): a lookup counts itself in `readers` and gives up
// (`busy`, nothing found, the binding looked up again later) while an observed mutator runs; a mutator counts itself in
// `writers` and waits for the lookups in flight (a few microseconds each) before the original runs. Writers do not exclude each
// other (as in the original), so no lock order is added between engine threads. The outermost observed mutator only (D4F250 ->
// D4EE10 nest on one thread); a lookup inside one on its own thread runs at once (none of the module's does: no module code runs
// inside the mutators). The engine's own slot-6
// lookups and the unobserved mutators above stay as in the original.
std::atomic<uint32_t> texture_registry_readers{},texture_registry_writers{};thread_local uint32_t texture_registry_depth{};
uintptr_t NativeTextureRegistryFind(uintptr_t resolver,const wchar_t* path,bool& busy) {
    const auto find=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const wchar_t*)>(image+0xD24490);
    if(texture_registry_depth)return find(resolver,path);
    texture_registry_readers.fetch_add(1);   // sequentially consistent with the writers' count (each side sees the other)
    if(texture_registry_writers.load()){texture_registry_readers.fetch_sub(1);busy=true;return 0;}
    const auto holder=find(resolver,path);texture_registry_readers.fetch_sub(1);return holder;
}
// P4c: how long a choice without the first candidate's holder is kept under an unchanged registry before it is looked up
// again (bounds K gap (c)).
constexpr uint64_t native_texture_fallback_ms=500;
// I-0071 (ER B, D-0195): the GX texture a holder finally holds. SprjGxTexture (vtable image+0x29559E8) forwards to its inner
// +0x10; a requested cap's holder holds SYSTEX_DUMMY's holder until the texture arrives (one level more), whose inner is a
// SprjDummyBlinkGxTexture (no native record); 0 when a holder holds nothing. `valid` false when `holder` itself is not a
// SprjGxTexture or a word is unreadable (the binding's holders are then looked up again).
uintptr_t NativeHolderInner(uintptr_t holder,bool* valid) noexcept {   // valid defaults to null (declared in 03_engine_cpu_owners.inc)
    uintptr_t at=holder;if(valid)*valid=true;
    for(int depth=0;depth<4 && at;++depth){uintptr_t table{};
        if(!Read(at,&table,8)){if(valid)*valid=false;return 0;}
        if(table!=image+0x29559E8){if(!depth && valid)*valid=false;return depth?at:0;}
        if(!Read(at+0x10,&at,8)){if(valid)*valid=false;return 0;}}
    return at;
}
// The holders of one binding's candidates (the programme's kept choice, or D24490 again: see TextureChoice).
// `busy` (C1, the step review's M1): a mutator ran during the lookup: nothing is kept and the holders found are not the binding's (the
// caller leaves its binding as it is, or unbound, and looks it up again later).
std::array<uintptr_t,4> NativeTextureHolders(const NativeModelLayout::Programs& programs,size_t index,uintptr_t resolver,uintptr_t root,bool* busy=nullptr) {
    const auto registry=texture_registry_generation.load(std::memory_order_acquire);const auto now_ms=GetTickCount64();
    NativeModelLayout::Programs::TextureChoice choice;
    {const std::lock_guard hold(programs.texture_choice_lock);choice=programs.texture_choices[index];}
    const auto scene=NativeKeptDrawResets();
    if(choice.root==root && choice.scene==scene && (choice.holders[0] || (choice.registry==registry && now_ms<choice.looked_up_ms+native_texture_fallback_ms))){
        kprofile::Add(kprofile::TextureFallbacksKept);return choice.holders;}
    const auto& candidates=programs.texture_lookups[index].candidates;
    choice={registry,root,{},now_ms,scene};bool gave_up{};
    for(size_t k=0;k<candidates.size() && k<choice.holders.size() && !gave_up;++k){
        const auto holder=NativeTextureRegistryFind(resolver,candidates[k].c_str(),gave_up);
        uintptr_t table{};choice.holders[k]=holder && Read(holder,&table,8) && table==image+0x29559E8?holder:0;}
    kprofile::Add(kprofile::TextureLookups);
    if(gave_up){if(busy)*busy=true;return {};}
    const std::lock_guard hold(programs.texture_choice_lock);programs.texture_choices[index]=choice;
    return choice.holders;
}
// The first candidate whose holder holds a resident native texture (as SprjMiddleGxTexture: the full package when valid, else
// its "_l" package), with every candidate's inner as read; null when none.
std::shared_ptr<const bootstrap::NativeTextureRecord> ResolveNativeTextureHolders(const std::array<uintptr_t,4>& holders,
    std::array<uintptr_t,4>& inners,size_t* chosen=nullptr) {
    std::shared_ptr<const bootstrap::NativeTextureRecord> found;
    for(size_t k=0;k<holders.size();++k){inners[k]=holders[k]?NativeHolderInner(holders[k]):0;
        if(found || !inners[k])continue;
        if(auto record=textures.Acquire(inners[k]);record && record->resident && record->resource && record->view){found=std::move(record);if(chosen)*chosen=k;}}
    return found;
}
using TextureRegistryCall=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
std::array<TextureRegistryCall,6> texture_registry_original{};
template<size_t Index> uintptr_t __fastcall ObserveTextureRegistry(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d,uintptr_t e,uintptr_t f) {
    // C1: the outermost observed mutator on this thread counts itself a writer and waits for the lookups in flight; the counts
    // and the depth restored on any exit (the step review's L7).
    struct Writer {bool outer;Writer():outer(!texture_registry_depth++){if(!outer)return;texture_registry_writers.fetch_add(1);
            while(texture_registry_readers.load())SwitchToThread();}
        ~Writer(){if(outer)texture_registry_writers.fetch_sub(1);--texture_registry_depth;}} writer;
    const auto result=texture_registry_original[Index](a,b,c,d,e,f);
    texture_registry_generation.fetch_add(1,std::memory_order_acq_rel);return result;
}
// Workstream K P4a: the path-only half of the texture admission (AdmitNativeModelCandidate), once per programme object.
void PlanNativeModelTextures(NativeModelLayout::Programs& programs) {
    programs.texture_choices.resize(programs.texture_bindings.size());
    for(const auto& recipe:programs.interfaces)if(const auto& pixel=recipe[1])for(const auto& resource:pixel->resources)
        if(resource.shader_register==56 && resource.input_type==2 && resource.name=="g_DepthTexture")programs.samples_depth56=true;
    for(const auto& recipe:programs.interfaces)for(const auto& shader:recipe)if(shader)for(const auto& resource:shader->resources)
        if(resource.input_type==2 && resource.shader_register>=47 && resource.shader_register<=51)programs.samples_snow|=uint8_t(1u<<(resource.shader_register-47));
    for(const auto& input:programs.texture_bindings){
        auto& lookup=programs.texture_lookups.emplace_back();
        const auto slash=input.path.find_last_of(L"/\\");lookup.leaf=input.path.substr(slash==std::wstring::npos?0:slash+1);
        if(const auto dot=lookup.leaf.find_last_of(L'.');dot!=std::wstring::npos)lookup.leaf.resize(dot);
        lookup.leaf_folded=lookup.leaf;for(auto& ch:lookup.leaf_folded)ch=static_cast<wchar_t>(towlower(ch));
        lookup.facegen=!lookup.leaf.empty() && _wcsicmp(lookup.leaf.c_str(),L"FG_face_gen")==0;
        if(input.declaration.slot==45)programs.names_slot45=true;
        if(input.declaration.slot>=30 && input.declaration.slot<94)programs.named_scene_slots|=1ull<<(input.declaration.slot-30);
        if(lookup.facegen || input.path.empty())continue;
        const auto variant=[&](const wchar_t* suffix){
            const auto dot=input.path.find_last_of(L'.');
            if(dot==std::wstring::npos || (slash!=std::wstring::npos && dot<slash))return input.path+suffix;
            return input.path.substr(0,dot)+suffix+input.path.substr(dot);};
        lookup.candidates={input.path,variant(L"_l")};
        if(input.path.find(L"_dol_0")!=std::wstring::npos)lookup.candidates.push_back(variant(L"_atlas000"));
    }
}
// Optimisation review (Native2200, the zone load's first frames): the interface reflection and the disassembly read analysis
// of a programme were run again for every layout that selects it (2.9 s of D3DDisassemble and 0.6 s of D3DReflect on the
// engine thread at the first m33 world frames, the 3.4 s hitch). Both are pure functions of the bytecode and the stage:
// they are kept per programme content (hashed, then compared byte for byte) for the process's life. A programme the
// decoder refuses is kept as refused, so its recipes stay unavailable as before.
struct NativeProgramAnalysis {ds3content::ShaderStage stage{};std::vector<uint8_t> bytes;std::optional<ds3content::ShaderInterface> shader;ds3reads::Reads reads;bool with_reads{};};
std::mutex native_program_analysis_lock;std::unordered_map<uint64_t,std::vector<std::shared_ptr<const NativeProgramAnalysis>>> native_program_analyses;

// The interface (and, with_reads, the conservative reads) of one programme; throws std::runtime_error as the decoder does.
std::shared_ptr<const NativeProgramAnalysis> AnalyzeNativeProgram(const ds3content::CpuShaderProgram& input,bool with_reads){
    uint64_t h=1469598103934665603ULL;const auto mix=[&](uint64_t v){h^=v;h*=1099511628211ULL;};
    mix(static_cast<uint64_t>(input.stage));mix(input.model_major);mix(input.bytecode.size());
    {const auto* p=input.bytecode.data();const size_t n=input.bytecode.size();size_t i=0;
        for(;i+8<=n;i+=8){uint64_t w;std::memcpy(&w,p+i,8);mix(w);}for(;i<n;++i)mix(p[i]);}
    {const std::lock_guard hold(native_program_analysis_lock);
        for(const auto& known:native_program_analyses[h])
            if(known->stage==input.stage && known->with_reads>=with_reads && known->bytes==input.bytecode){
                native_program_analysis_hits.fetch_add(1,std::memory_order_relaxed);
                if(!known->shader)throw std::runtime_error("native programme interface unavailable (kept)");
                return known;}}
    LARGE_INTEGER start{},end{};QueryPerformanceCounter(&start);
    auto entry=std::make_shared<NativeProgramAnalysis>();entry->stage=input.stage;entry->bytes=input.bytecode;entry->with_reads=with_reads;
    try{entry->shader=ds3content::InspectShaderInterface(input);if(with_reads)entry->reads=ds3reads::AnalyzeConservative(input.bytecode);}
    catch(const std::runtime_error&){entry->shader.reset();}
    QueryPerformanceCounter(&end);native_program_analysis_ticks.fetch_add(uint64_t(end.QuadPart-start.QuadPart),std::memory_order_relaxed);
    native_program_analysis_misses.fetch_add(1,std::memory_order_relaxed);
    // I-0100 tranche 7: two admission workers analysing one new programme at once kept two entries, so a programme's interface
    // had two addresses (i100-t7h-verify-settlement-n1: recipe-verify.flag found the DptA recipe's interface differing by address
    // in 4,002 of 4,096 takes): the first entry kept is the one every caller gets.
    {const std::lock_guard hold(native_program_analysis_lock);auto& kept=native_program_analyses[h];
        for(const auto& known:kept)if(known->stage==entry->stage && known->with_reads>=entry->with_reads && known->bytes==entry->bytes){
            if(!known->shader)throw std::runtime_error("native programme interface unavailable (kept)");
            return known;}
        kept.push_back(entry);}
    if(!entry->shader)throw std::runtime_error("native programme interface unavailable");
    return entry;
}
// I-0100 tranche 7 (RESULT tranche 7 step 1: i100-t7a-settlement-n1, the selections and the analyses' lookups were 58 % of the
// programme sets' build time in the camera turn, 653 sets built for its first sights): a layout's flat recipes (their programme
// selections, interfaces, reads and loose pixel programmes) depend only on the effect, the programme path, the layout's type and
// instancing, the channel preference and the loaded programme repositories, so they are kept per that key, the shaders'
// admissions and retirements counted in it (shader_registry_changes: what FindLoadedProgramme's answers depend on; the effect's
// own generation covers its programmes; step review L3), and every layout of the same recipe set takes them as they are. Entries
// of earlier counts are dropped beyond 4,096. A set made while a loose programme it asked for was absent keeps that programme's
// name and asks FindLoadedProgramme again for it at every take (that lookup's own 250 ms rule for an absence, Native2202: the
// engine may link a repository node after its shader's admission); found now, the set is made again (step review M1; a set
// expiring after 250 ms instead was made again for most families, which have no phantom programme: i100-t7e-settlement-n1).
// The block plans (PlanNativeModelBlocks) are kept in the set they were made from, by authored material (its MTD values: the
// cbMtdParam fallbacks), so a layout's plans always come from the recipes it took (step review M2).
struct NativeRecipeSet {
    std::array<ds3content::ModelProgramSelection,native_recipe_count> recipes{};
    std::array<std::array<std::shared_ptr<const ds3content::ShaderInterface>,2>,native_recipe_count> interfaces;
    std::array<std::array<std::shared_ptr<const ds3reads::Reads>,2>,native_recipe_count> reads;
    std::shared_ptr<const NativeShader> velocity_common_pixel;std::array<std::shared_ptr<const NativeShader>,2> phantom_loose_pixel;
    std::vector<std::wstring> absent;   // the loose programmes asked for and not found (FindLoadedProgramme, repository 0x4792BF8)
    mutable std::mutex plans_lock;
    mutable std::unordered_map<uint64_t,std::shared_ptr<const NativeModelLayout::Programs::BlockPlans>> plans;   // by authored generation
    bool Current() const {for(const auto& name:absent)if(FindLoadedProgramme(0x4792BF8,name.c_str(),0u))return false;return true;}
};
struct NativeRecipeSetKey {
    uint64_t registry{},effect{};std::wstring path;uint32_t type{};bool instanced{},prefer_channel{};
    bool operator==(const NativeRecipeSetKey&) const=default;
};
struct NativeRecipeSetHash {size_t operator()(const NativeRecipeSetKey& k) const noexcept {
    uint64_t h=1469598103934665603ULL;const auto mix=[&](uint64_t v){h^=v;h*=1099511628211ULL;};
    mix(k.registry);mix(k.effect);mix(k.type);mix(uint64_t(k.instanced)|uint64_t(k.prefer_channel)<<1);for(const auto ch:k.path)mix(ch);return size_t(h);}};
std::mutex native_recipe_set_lock;
std::unordered_map<NativeRecipeSetKey,std::shared_ptr<const NativeRecipeSet>,NativeRecipeSetHash> native_recipe_sets;
std::shared_ptr<const NativeModelLayout::Programs> AcquireNativeModelPrograms(
    const std::shared_ptr<const NativeModelMaterial>& material,const std::shared_ptr<NativeModelLayout>& layout,
    bool prefer_channel) {
    // Workstream K P6: the layout's last successful choice stands while no registry it reads has changed (failures are
    // never kept).
    const auto generation=program_registry_generation.load(std::memory_order_acquire);
    const auto slot=prefer_channel?1u:0u;
    {NativeModelLayout::ProgramsChoice choice;
        {const std::lock_guard hold(layout->programs_lock);choice=layout->choices[slot];}
        // The material is held weakly: a freed material whose address is reused never matches (review K, finding 2).
        if(choice.programs && choice.generation==generation && choice.material.lock()==material && choice.authored==material->authored.get())return choice.programs;}
    // I-0100 tranche 7 (crossing-observe.flag): the build's parts timed (native_programs_build_ticks).
    static const bool split=ModuleFlag(L"crossing-observe.flag");
    LARGE_INTEGER split_at{};if(split)QueryPerformanceCounter(&split_at);
    const auto part=[&](size_t index){if(!split)return;LARGE_INTEGER now{};QueryPerformanceCounter(&now);
        native_programs_build_ticks[index].fetch_add(uint64_t(now.QuadPart-split_at.QuadPart),std::memory_order_relaxed);split_at=now;};
    // D24430 is the genuine CPU authored-name lookup, returning the loaded
    // effect prefix. Only an actual native registry owner may satisfy it.
    std::shared_ptr<const NativeMaterial> authored;
    // 1002 witness: why a selected mesh has no programme family (first misses).
    static LONG64 missing_authored{},missing_effect{};
    {const std::lock_guard hold(material_lock);const auto found=materials.find(material->authored_identity);
        if(found==materials.end() || found->second->incarnation!=material->authored->incarnation){
            if(InterlockedIncrement64(&missing_authored)<=4)Log("{\"event\":\"native_model_programs_missing\",\"reason\":\"authored_material\"}\n");
            return {};}
        authored=found->second;}
    const auto owner=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const wchar_t*)>(image+0xD24430)
        (layout->resolver,authored->shader.c_str());
    std::shared_ptr<const NativeEffect> effect;std::wstring program_path=authored->shader;
    {const std::lock_guard hold(effect_lock);const auto found=effects.find(owner);
        if(found!=effects.end())effect=found->second;}
    part(0);
    if(!effect && !owner){
        // 82220 (prog1009): when the resolver has no loaded group (+48 = D24430)
        // it applies fixed renames and resolves the group through +60 = D245A0,
        // whose D467E0 composes it by name from loaded programmes ("<group>_<pass>"
        // via the old device's name table). Natively the loaded programmes are the
        // admitted effects' programmes: use the effect that owns "<group>_Gbuf..."
        // programmes. No old device, group object or shader handle is created.
        const auto leaf=[&](const std::wstring& path){
            const auto name=reinterpret_cast<const wchar_t*(__fastcall*)(const wchar_t*)>(image+0xD1760)(path.c_str());
            const auto extension=reinterpret_cast<const wchar_t*(__fastcall*)(const wchar_t*)>(image+0xD16E0)(name);
            return extension?std::wstring(name,extension):std::wstring(name);};
        const auto shader=leaf(authored->shader),mtd=leaf(material->mtd);
        std::wstring group=shader;
        if(shader==L"GXFlver_ColDifSpcBumpIbl_BlC")group=L"GXFlver_ColDifSpcBumpIbl_Bloody_Cloth";
        else if(mtd==L"Displacement" || shader==L"Displacement")group=L"GXFlver_ColDifSpcBumpIbl";
        else if(mtd==L"MixDisplacement" || shader==L"MixDisplacement")group=L"GXFlver_ColDifSpcBumpIblMixDisp";
        else if(mtd==L"Specular" || shader==L"Specular")group=L"GXFlver_ColDifNewSpcBumpIbl";
        const auto prefix=group+L"_Gbuf";size_t owners_found{};
        // I-0100 tranche 7 (i100-t7-recon-archdragon-n1: this scan of every effect's programmes was the largest part of the sets
        // built in play at Archdragon Peak, 192 ms of 456): its answer kept per group while the effects are the same (effects_changes;
        // the registry's generation moves with every material DS3 streams there, i100-t7d-archdragon-n1). Held weakly: a retired
        // effect is not kept alive by this answer (step review L1). A group no effect owns (the composed groups, found below) is an
        // answer too (i100-t7e-archdragon-n1: GXFlver_SkinSSSIbl_Bloody scanned again at every set). Under effect_lock, as the scan.
        struct ByName {uint64_t changes{};std::weak_ptr<const NativeEffect> effect;bool owned{};};
        static std::unordered_map<std::wstring,ByName> by_name;
        bool by_name_kept=false;
        {const std::lock_guard hold(effect_lock);
            if(const auto found=by_name.find(group);found!=by_name.end() && found->second.changes==effects_changes){
                effect=found->second.effect.lock();by_name_kept=!found->second.owned || effect;}
            if(!by_name_kept){
                for(const auto& [key,candidate]:effects){(void)key;
                    if(std::any_of(candidate->program_names.begin(),candidate->program_names.end(),[&](const auto& program){
                        return program.name.size()>prefix.size() && _wcsnicmp(program.name.c_str(),prefix.c_str(),prefix.size())==0;})){
                        ++owners_found;if(!effect)effect=candidate;}}
                if(by_name.size()>=1024)by_name.clear();by_name.insert_or_assign(group,ByName{effects_changes,effect,effect!=nullptr});}}
        size_t loose{};
        if(!effect)effect=ComposeNativeShaderGroup(group,loose);
        static LONG64 composed{},absent{};
        const auto count=InterlockedIncrement64(effect?&composed:&absent);
        if(count<=12){std::string text;for(const auto ch:group)text.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
            // (effects_with_programmes -1: the answer was kept, the effects not counted again.)
            char row[512]{};sprintf_s(row,"{\"event\":\"native_model_programs_by_name\",\"group\":\"%.300s\",\"effects_with_programmes\":%lld,\"loose_programmes\":%zu,\"composed_programmes\":%zu,\"resolved\":%s,\"kept\":%s,\"count\":%lld}\n",
                text.c_str(),by_name_kept?-1ll:static_cast<long long>(owners_found),loose,effect?effect->programs.size():0,effect?"true":"false",by_name_kept?"true":"false",count);Log(row);}
        if(!effect)return {};
        program_path=group+L".spx";
    }
    part(1);
    if(!effect){
        const auto count=InterlockedIncrement64(&missing_effect);
        if(count<=12){std::string shader;for(const auto ch:authored->shader)shader.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
            char row[512]{};sprintf_s(row,"{\"event\":\"native_model_programs_missing\",\"reason\":\"effect_not_registered\",\"shader\":\"%.300s\",\"count\":%lld}\n",shader.c_str(),count);Log(row);}
        return {};}
    const std::lock_guard hold(layout->programs_lock);
    part(2);
    auto& kept=layout->programs[slot];
    // I-0120 C6: and while every loose programme its recipes asked for and did not find is still absent (NativeRecipeSet's
    // Current, for the set this layout took: GXFlver_Common_Velo, <family>_Phantom_Fwd linked after the first build). Asked again
    // only after a shader admission or retirement or a second after the last asking (the final review's M1: not at every registry
    // move); this path itself runs only when the layout's choice is asked again (the program registry moved).
    const auto still_absent=[&]{if(kept->absent.empty())return true;
        const auto changes=shader_registry_changes.load(std::memory_order_acquire);const auto now=GetTickCount64();
        if(changes==kept->absent_changes && now-kept->absent_at<1000)return true;
        kept->absent_changes=changes;kept->absent_at=now;
        return std::none_of(kept->absent.begin(),kept->absent.end(),[](const std::wstring& name){return FindLoadedProgramme(0x4792BF8,name.c_str(),0u)!=nullptr;});};
    if(kept && kept->effect==effect && kept->authored==authored && kept->prefer_channel==prefer_channel && still_absent()){
        layout->choices[slot]={generation,material,material->authored.get(),kept};
        if(split)native_programs_kept.fetch_add(1,std::memory_order_relaxed);
        return kept;}
    auto prepared=std::make_shared<NativeModelLayout::Programs>();prepared->effect=std::move(effect);prepared->program_path=program_path;prepared->hair_family=program_path.find(L"Hair")!=std::wstring::npos;
    prepared->authored=std::move(authored);
    prepared->prefer_channel=prefer_channel;
    ds3content::ModelDamageParameters damage;
    if(ds3content::ReadModelDamageParameters(material->gx_records,prepared->effect->parameter_declarations,
        NativeBuiltinModelParameters(),prepared->authored->numeric_parameters,
        prepared->authored->unsupported_parameters.empty(),damage)==ds3content::ModelGeneratedStatus::Ready)
        prepared->damage_parameters=std::move(damage);
    if(!prepared->effect->declarations.empty()){
        ds3content::ModelGeneratedParameters generated;
        if(ds3content::ReadModelGeneratedParameters(prepared->effect->declarations,material->gx_records,generated)==
           ds3content::ModelGeneratedStatus::Ready)prepared->generated_parameters=std::move(generated);
    }
    prepared->texture_bindings_available=ds3content::PrepareModelTextureBindings(material->texture_inputs,
        !prepared->effect->declarations.empty(),prepared->effect->texture_declarations,
        NativeBuiltinModelTextures(),prepared->texture_bindings);
    if(prepared->authored->unsupported_parameters.empty()){
        const auto name=reinterpret_cast<const wchar_t*(__fastcall*)(const wchar_t*)>(image+0xD1760)(prepared->authored->shader.c_str());
        const auto extension=reinterpret_cast<const wchar_t*(__fastcall*)(const wchar_t*)>(image+0xD16E0)(name);
        const std::wstring family=extension?std::wstring(name,extension):std::wstring(name);
        prepared->shader_family=family;
        ds3content::ModelMaterialPolicy policy;
        if(ds3content::PrepareModelMaterialPolicy(family,prepared->effect->parameter_declarations,
            NativeBuiltinModelParameters(),prepared->authored->numeric_parameters,policy))prepared->policy=policy;
        // Fable review (the Handmaid chair, M[ARSN]_e with g_AlphaRef 128 drawn without its alpha test): once per family, the
        // policy-bearing MTD parameters that PrepareModelMaterialPolicy skips because the effect's SPX declares the same name.
        {static std::mutex lock;static std::set<std::wstring> seen;std::string skipped;
            for(const auto& value:prepared->authored->numeric_parameters){
                if(value.name!="g_AlphaRef" && value.name!="g_BlendMode" && value.name!="g_GX_bDisableDepthWrite" && value.name!="g_GX_bDisableDecal" && value.name!="g_GX_MaterialType")continue;
                std::wstring wide;for(const unsigned char ch:value.name)wide.push_back(ch);
                if(std::any_of(prepared->effect->parameter_declarations.begin(),prepared->effect->parameter_declarations.end(),[&](const auto& row){return row.name==wide;}))
                    skipped+=(skipped.empty()?"":",")+value.name+"="+std::to_string(value.words[0]);}
            // and, once per family, a g_AlphaRef the policy left at 0: its type and words as the game's MTD getter returned them.
            for(const auto& value:prepared->authored->numeric_parameters)if(value.name=="g_AlphaRef" && prepared->policy && prepared->policy->alpha_ref==0){
                static std::mutex zero_lock;static std::set<std::wstring> zero_seen;bool fresh{};{const std::lock_guard held(zero_lock);fresh=zero_seen.size()<64 && zero_seen.insert(family).second;}
                if(fresh){std::string ascii;for(const auto ch:family)ascii.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                    std::string mtd;for(const auto ch:prepared->authored->shader)if(mtd.size()<120)mtd.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                    char row[480]{};sprintf_s(row,"{\"event\":\"native_policy_alpha_ref_zero\",\"family\":\"%.120s\",\"shader\":\"%s\",\"type\":%u,\"words\":[%u,%u,%u,%u],\"declared\":%zu,\"numeric\":%zu}\n",
                        ascii.c_str(),mtd.c_str(),value.type,value.words[0],value.words[1],value.words[2],value.words[3],prepared->effect->parameter_declarations.size(),prepared->authored->numeric_parameters.size());Log(row);}}
            if(!skipped.empty()){bool fresh{};{const std::lock_guard held(lock);fresh=seen.size()<64 && seen.insert(family).second;}
                if(fresh){std::string ascii;for(const auto ch:family)ascii.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                    const std::string row="{\"event\":\"native_policy_declared_skip\",\"family\":\""+ascii.substr(0,120)+"\",\"skipped\":\""+skipped+"\",\"alpha_ref\":"+std::to_string(prepared->policy?prepared->policy->alpha_ref:0u)+"}\n";Log(row.c_str());}}}
    }
    part(3);
    // Exact current SPX declarations + actual typed MTD words. Keep immutable
    // base producers with both owner leases; current animation and shader-used
    // byte coverage still govern each draw's later constant publication.
    for(const auto& declaration:prepared->effect->parameter_declarations){
        bool available=false;
        for(const auto& value:prepared->authored->numeric_parameters){
            ds3content::PreparedModelParameter parameter;
            if(ds3content::PrepareModelNumericParameter(declaration,value,parameter)){
                prepared->numeric_parameters.push_back(std::move(parameter));available=true;break;}
        }
        if(!available)prepared->unavailable_parameters.push_back(declaration.name);
    }
    for(const auto& value:prepared->authored->numeric_parameters){
        std::wstring name;bool ascii=true;for(const unsigned char ch:value.name){if(ch>=128){ascii=false;break;}name.push_back(ch);}
        if(!ascii)continue;
        if(std::any_of(prepared->effect->parameter_declarations.begin(),prepared->effect->parameter_declarations.end(),
            [&](const auto& declaration){return declaration.name==name;}))continue;
        for(const auto& declaration:NativeBuiltinModelParameters())if(declaration.name==name){
            ds3content::PreparedModelParameter parameter;
            if(ds3content::PrepareModelBuiltinParameter(declaration,value,parameter))
                prepared->numeric_parameters.push_back(std::move(parameter));
            else prepared->unavailable_parameters.push_back(name);
            break;
        }
    }
    part(4);
    // I-0100 tranche 7: the flat recipes from their kept set (NativeRecipeSet, above), else selected and analysed below and kept.
    const auto shader_changes=shader_registry_changes.load(std::memory_order_acquire);
    const NativeRecipeSetKey recipe_key{shader_changes,prepared->effect->generation,prepared->program_path,layout->type,layout->instanced!=0,prefer_channel};
    std::shared_ptr<const NativeRecipeSet> recipe_set;
    {const std::lock_guard recipe_hold(native_recipe_set_lock);
        if(const auto found=native_recipe_sets.find(recipe_key);found!=native_recipe_sets.end())recipe_set=found->second;}
    if(recipe_set && !recipe_set->Current())recipe_set.reset();   // outside the lock: its absent loose programmes asked again
    std::vector<std::wstring> absent;   // the loose programmes asked for and not found (NativeRecipeSet, above)
    if(recipe_set){prepared->recipes=recipe_set->recipes;prepared->interfaces=recipe_set->interfaces;prepared->reads=recipe_set->reads;
        prepared->velocity_common_pixel=recipe_set->velocity_common_pixel;prepared->phantom_loose_pixel=recipe_set->phantom_loose_pixel;
        if(split)native_recipe_sets_taken.fetch_add(1,std::memory_order_relaxed);part(5);}
    // The flat recipes selected and analysed into `into` (`prepared`, or recipe-verify.flag's uncached copy below), the loose
    // programmes asked for and absent named in `names`.
    const auto select_flat=[&](NativeModelLayout::Programs& into,std::vector<std::wstring>& names){
        // 1186: pass 1 (Velo) recipes too; its plain pixel programme is the shared loose GXFlver_Common_Velo (AA630).
        // D-0180 C2: and pass 2 (Dpt, DptA for the alpha-tested variant) for the sun shadow's casters.
        // WATER-TESS-WAVE1-READY WT-1: and pass 12 (WatDpt, one pixel variant) for the water rows 0x24.
        for(const uint32_t pass:{6u,7u,0u,1u,native_phantom_forward_pass,2u,12u})for(uint32_t pixel=0;pixel<(pass==12?1u:pass==native_phantom_forward_pass || pass==2?2u:3u);++pixel){
            const bool phantom=pass==native_phantom_forward_pass;
            const ds3content::ModelProgramIntent intent{phantom?0u:pass,layout->type,0,pixel,layout->instanced!=0,prefer_channel,phantom};
            into.recipes[NativeRecipe(pass,pixel)]=ds3content::SelectModelPrograms(into.program_path,
                intent,into.effect->program_names);
            const auto index=NativeRecipe(pass,pixel);auto& recipe=into.recipes[index];
            // A9EF0 without the family's own phantom programme: the loaded repository's <family>_Phantom_Fwd[_[A]] with the Fwd VS.
            if(phantom && recipe.status!=ds3content::ModelProgramStatus::Resolved){
                auto forward=ds3content::SelectModelPrograms(into.program_path,{0u,layout->type,0,pixel,layout->instanced!=0,prefer_channel,false},
                    into.effect->program_names);
                const auto slash=into.program_path.find_last_of(L"/\\");auto family=into.program_path.substr(slash==std::wstring::npos?0:slash+1);
                if(family.size()>4 && _wcsicmp(family.c_str()+family.size()-4,L".spx")==0)family.resize(family.size()-4);
                if(forward.status==ds3content::ModelProgramStatus::Resolved && !family.empty()){
                    const auto name=family+L"_Phantom_Fwd"+(pixel?L"_[A]":L"");
                    if(auto loose=FindLoadedProgramme(0x4792BF8,name.c_str(),0u)){
                        into.phantom_loose_pixel[pixel]=std::move(loose);forward.shared_pixel=true;forward.pixel=0;recipe=forward;}
                    else names.push_back(name);}}
            if(recipe.status==ds3content::ModelProgramStatus::Resolved && recipe.shared_pixel && !phantom){
                if(!into.velocity_common_pixel)into.velocity_common_pixel=FindLoadedProgramme(0x4792BF8,ds3content::velocity_common_pixel_programme,0u);
                if(!into.velocity_common_pixel){recipe.status=ds3content::ModelProgramStatus::Missing;
                    if(std::find(names.begin(),names.end(),ds3content::velocity_common_pixel_programme)==names.end())names.emplace_back(ds3content::velocity_common_pixel_programme);}}
            part(5);
            if(recipe.status==ds3content::ModelProgramStatus::Resolved){
                for(size_t stage=0;stage<2;++stage){
                    ds3content::CpuShaderProgram input;input.stage=stage?ds3content::ShaderStage::Pixel:ds3content::ShaderStage::Vertex;
                    input.model_major=5;input.bytecode=into.ProgramBytes(index,stage);
                    // Existing owned DXBC decoder. Unsupported reflection remains
                    // unavailable for this recipe; no substituted layout is used.
                    try{const auto analysis=AnalyzeNativeProgram(input,true);into.interfaces[index][stage]={analysis,&*analysis->shader};
                        into.reads[index][stage]={analysis,&analysis->reads};}
                    catch(const std::runtime_error&){into.interfaces[index][stage].reset();}
                }
            }
            part(6);
        }
    };
    if(!recipe_set)select_flat(*prepared,absent);
    // I-0100 tranche 7 (final review M3): with recipe-verify.flag every taken set is also made uncached and compared (its
    // selections, the kept analyses each interface and read points to, the loose pixel programmes): a difference is a row,
    // native_recipe_set_mismatch (the first 16), and a count in native_recipe_set_verified every 4,096 takes.
    static const bool verify_recipes=ModuleFlag(L"recipe-verify.flag");
    if(verify_recipes && recipe_set){NativeModelLayout::Programs fresh_set;fresh_set.effect=prepared->effect;fresh_set.program_path=prepared->program_path;
        fresh_set.authored=prepared->authored;std::vector<std::wstring> fresh_absent;select_flat(fresh_set,fresh_absent);
        int differing=-1;unsigned what{};   // what: 1 the selection, 2 an interface, 4 a read, 8 a loose pixel programme
        for(size_t i=0;i<native_recipe_count && differing<0;++i){const auto& a=fresh_set.recipes[i];const auto& b=prepared->recipes[i];
            what=(a.status!=b.status || a.vertex!=b.vertex || a.pixel!=b.pixel || a.channel!=b.channel || a.shared_pixel!=b.shared_pixel || a.hull!=b.hull ||
               a.domain!=b.domain || a.tessellated!=b.tessellated?1u:0u)|(fresh_set.interfaces[i]!=prepared->interfaces[i]?2u:0u)|(fresh_set.reads[i]!=prepared->reads[i]?4u:0u);
            if(what)differing=int(i);}
        if(differing<0 && (fresh_set.velocity_common_pixel!=prepared->velocity_common_pixel || fresh_set.phantom_loose_pixel!=prepared->phantom_loose_pixel)){differing=int(native_recipe_count);what=8;}
        static std::atomic<uint64_t> verified{},mismatches{};const auto n=verified.fetch_add(1,std::memory_order_relaxed)+1;
        if(differing>=0 && mismatches.fetch_add(1,std::memory_order_relaxed)<16){char row[192]{};
            sprintf_s(row,"{\"event\":\"native_recipe_set_mismatch\",\"recipe\":%d,\"what\":%u,\"verified\":%llu}\n",differing,what,static_cast<unsigned long long>(n));Log(row);}
        if(!(n%4096)){char row[160]{};sprintf_s(row,"{\"event\":\"native_recipe_set_verified\",\"takes\":%llu,\"mismatches\":%llu}\n",
            static_cast<unsigned long long>(n),static_cast<unsigned long long>(mismatches.load()));Log(row);}}
    // Kept (replacing one another worker kept meanwhile): this layout's plans then come from the set it holds, the one it made
    // (step review M2).
    if(!recipe_set){auto kept_set=std::make_shared<NativeRecipeSet>();kept_set->recipes=prepared->recipes;kept_set->interfaces=prepared->interfaces;
        kept_set->reads=prepared->reads;kept_set->velocity_common_pixel=prepared->velocity_common_pixel;kept_set->phantom_loose_pixel=prepared->phantom_loose_pixel;
        kept_set->absent=std::move(absent);recipe_set=kept_set;
        // (Entries of earlier shader counts first; if that frees nothing, the sets of retired effects of this count: all of them,
        // the next layouts making theirs again. Fixes' step review L1. The dropped sets are released after the lock, final review L3.)
        decltype(native_recipe_sets) dropped;
        {const std::lock_guard recipe_hold(native_recipe_set_lock);
            if(native_recipe_sets.size()>=4096){
                for(auto at=native_recipe_sets.begin();at!=native_recipe_sets.end();){if(at->first.registry!=shader_changes)dropped.insert(native_recipe_sets.extract(at++));else ++at;}
                if(native_recipe_sets.size()>=4096)dropped.merge(native_recipe_sets);}
            native_recipe_sets.insert_or_assign(recipe_key,std::move(kept_set));}}
    prepared->absent=recipe_set->absent;   // C6: asked again before this layout's kept programmes are returned
    // W7 TS-2b (WATER-TESS-WAVE1-READY 3.4): with native-tess.flag, a material whose GX04/GX90 item asks for PN tessellation
    // (mode 2, no displacement: deformation 3) gets the `_PN` recipes of the G-buffer and forward passes, with their hull and
    // domain interfaces and the material's cbPNTriangles block. Recipes that do not resolve stay unresolved: the draw is then
    // the flat recipe of the same pass (W0a-6).
    if(NativeTessellationEnabled())
        if(const auto tess=NativeMaterialTess(material->gx_records);tess && tess->mode==2 && !tess->displacement){
            auto block=std::make_shared<std::vector<uint8_t>>(0x30);
            if(ds3native::tess::PnTriangles(*tess,1.0f,block->data())){prepared->pn_triangles=std::move(block);
                for(const uint32_t pass:{6u,0u})for(uint32_t pixel=0;pixel<3;++pixel){
                    const auto index=NativeTessRecipe(pass,pixel);auto& recipe=prepared->recipes[index];
                    recipe=ds3content::SelectModelPrograms(prepared->program_path,
                        {pass,layout->type,3,pixel,layout->instanced!=0,prefer_channel,false},prepared->effect->program_names);
                    if(recipe.status!=ds3content::ModelProgramStatus::Resolved)continue;
                    if(!recipe.tessellated || recipe.shared_pixel){recipe.status=ds3content::ModelProgramStatus::Missing;continue;}
                    for(size_t stage=0;stage<4;++stage){
                        ds3content::CpuShaderProgram input;input.stage=stage==0?ds3content::ShaderStage::Vertex:stage==1?ds3content::ShaderStage::Pixel:
                            stage==2?ds3content::ShaderStage::Hull:ds3content::ShaderStage::Domain;
                        input.model_major=5;input.bytecode=prepared->ProgramBytes(index,stage);
                        try{if(stage<2){const auto analysis=AnalyzeNativeProgram(input,true);prepared->interfaces[index][stage]={analysis,&*analysis->shader};
                                prepared->reads[index][stage]={analysis,&analysis->reads};}
                            else prepared->tess_interfaces[index][stage-2]=AnalyzeNativeProgram(input,false)->shader;}
                        catch(const std::runtime_error&){prepared->interfaces[index][0].reset();prepared->interfaces[index][1].reset();
                            prepared->tess_interfaces[index][0].reset();prepared->tess_interfaces[index][1].reset();
                            recipe.status=ds3content::ModelProgramStatus::Missing;break;}
                    }
                }
            }
        }
    part(6);   // (the tessellated recipes, native-tess.flag)
    // I-0100 tranche 7: the block plans of this recipe set and authored material, kept as the recipes are (not with the tessellated
    // recipes, whose interfaces are the material's own).
    if(prepared->pn_triangles)prepared->block_plans=PlanNativeModelBlocks(*prepared);
    else{const auto authored_generation=prepared->authored->generation;
        {const std::lock_guard plan_hold(recipe_set->plans_lock);if(const auto found=recipe_set->plans.find(authored_generation);found!=recipe_set->plans.end())prepared->block_plans=found->second;}
        if(!prepared->block_plans){auto made=PlanNativeModelBlocks(*prepared);decltype(recipe_set->plans) dropped_plans;   // released after the lock
            const std::lock_guard plan_hold(recipe_set->plans_lock);if(recipe_set->plans.size()>=256)dropped_plans.swap(recipe_set->plans);
            prepared->block_plans=recipe_set->plans.try_emplace(authored_generation,std::move(made)).first->second;}}
    part(7);PlanNativeModelTextures(*prepared);
    part(8);if(split)native_programs_builds.fetch_add(1,std::memory_order_relaxed);
    kprofile::Add(kprofile::ProgramsBuilt);
    kept=prepared;layout->choices[slot]={generation,material,material->authored.get(),kept};return prepared;
}
// 983/994 witness: histogram of unsupported selected classes and, for
// SprjModelDispEntity (29489A0, map pieces), the classes of its child list +90
// (circular list, payload at node+10) that its emit D04B90 calls through +58.
std::mutex unsupported_selection_lock;
std::map<uint64_t,uint64_t> unsupported_selection_classes,disp_child_classes;
void SampleNativeUnsupportedSelection(uintptr_t entity,uintptr_t table) noexcept {
    const auto rva=[](uintptr_t a){return static_cast<uint64_t>(a>=image && a<image+0x6000000?a-image:a);};
    const std::lock_guard hold(unsupported_selection_lock);
    if(++unsupported_selection_classes[rva(table)]==1 && unsupported_selection_classes.size()<=48){char row[160]{};
        sprintf_s(row,"{\"event\":\"native_selected_class_first\",\"table_rva\":%llu}\n",rva(table));Log(row);}
    if(table!=image+0x29489A0)return;
    uintptr_t head{},node{};if(!Read(entity+0x90,&head,8) || !head || !Read(head,&node,8))return;
    for(uint32_t n=0;node && node!=head && n<64;++n){uintptr_t child{},child_table{};
        if(!Read(node+0x10,&child,8))break;
        if(child && Read(child,&child_table,8) && ++disp_child_classes[rva(child_table)]==1 && disp_child_classes.size()<=48){
            uintptr_t emit{};Read(child_table+0x58,&emit,8);char row[224]{};
            sprintf_s(row,"{\"event\":\"native_disp_child_class_first\",\"child_table_rva\":%llu,\"emit58_rva\":%llu}\n",rva(child_table),rva(emit));Log(row);}
        if(!Read(node,&node,8))break;}
}
// One selected model candidate: owner fields resolved per selected class
// (995-1001). SprjModelDrawEntity owns everything itself; for a map piece the
// SprjModelDispEntity owns fade, group and eligibility and each SprjFD4ModelItem
// child owns its model, placement, pose, stream source and light object.
struct NativeSelectionCandidate {
    uintptr_t entity{},model{},group{},animation{},source_field{},instance_field{},pose{},parameter_animations{};
    uintptr_t fade_owner{},face_owner{};ingress::ModelFadeLayout fade_layout;
    std::array<float,12> placement{};
    bool geometry_suppressed{},neutral_instance_decal{};
    uintptr_t draw_colour{};   // 1169, I-0140: the address of a disp item's or an ASM part's 16-byte draw colour, 0 = the group's own
    // Equipment hide (TestAnim white shins, Reference1908): CFDE70 draws a ChrAsm part with a stack copy of the group whose
    // capability words +0xC0/+0xC8 are the part descriptor's first 16 bytes OR bit 127 (5278C0(0x7F), 5B0800), not the shared
    // group's; 63CB0 tests each material's #NN# bit against them. 0 = the group's own.
    uintptr_t capability_source{};
    uintptr_t stream_patch{};  // 1177, I-0140: a disp item's FaceGen vertex stream patch [item+758] (D3DA20 descriptor +28)
    uintptr_t cloth_set{};     // 1184: the dynamic mesh set of this draw's cloth (MENUS-MAP 65), 0 = none
    uintptr_t cloth_instance{};   // 1185: its cloth instance I (DD5FD0 object)
    // 1186 (I-0067 VELOCITY-SKINNING §7): the velocity byte its emit copies to the draw (+0xD -> descriptor byte 1):
    // D079C0 E+0xBA5 && E+0xBA6 && E+0x184 (SprjModelDrawEntity), CFDE70 E+0xBB0 bits 6 and 7 && E+0x184 (ChrAsm parts),
    // D3DA20 the SprjModelDispEntity's +0x388 bit 1 (SprjFD4ModelItem children: map pieces and objects).
    bool velocity{};
    // I-0113 tranche 10d (57A20:657-720): the descriptor's byte +0x10, which makes a category-0x29 draw record a depth pre-pass
    // (preparation 1, Dpt) before its colour row: CFDE70 the part's flags +0x38 bit 0x20 with the entity's +0xBB0 bit 8 (the hair),
    // D079C0 the entity's +0xBA7, D3DA20 the disp entity's +0x388 bit 2.
    bool fade_prepass{};
    // I-0113 tranche 14: the box (8 floats, the minimum then the maximum) whose centre is the position 5C0E0 chooses the region IBL
    // at: D3DA20 the item's instance +0x80 (D17190), D079C0 the entity's +0xB80 (vt+0x18 -> +0x98 D07E10), their .w taken as 1;
    // CFDE70 the part's owner +0xAD0 (its vt+0x18 -> +0x98 D01A00), its .w the box's own. 0: none.
    uintptr_t region_box{};bool region_box_w{};
    // I-0113 tranche 15: the order key's bias from that box (world/sort_key.h): 1 half its depth extent (D3DA20, D079C0), 2 the
    // length of its half diagonal (CFDE70); plus the float at sort_extra (D079C0 the entity's +0xBA0, CFDE70 the owner's +0xAF0;
    // 0: none). 0: the emitter's key is not known.
    uint8_t sort_rule{};uintptr_t sort_extra{};
    // P4 (PHANTOMS-SIGNS-NATIVE §3): CFDE70 draws a ChrAsm part whose descriptor +0x44 (short) is a PhantomParam id with an
    // existing row with a stack copy of the group: +0x87 = 1, mulCol.w *= row fade (+0x24), +0x84 = row kind (+0x2C), kind 0
    // giving 1 when the faded alpha is below 1 and 0xFF otherwise. Native applies the same to its copy of the bytes.
    bool phantom{};uint8_t phantom_kind{};float phantom_fade{1.f};
    // E03 (I-0105): a model particle's FLVER draw (kind 3), its group the submission's draw state and its pose the submission's;
    // the entity and pose fields hold its key (AppendNativeEffectCandidates: no engine address), no engine entity.
    const NativeEffectModel* effect{};
};
// I-0071 point 3: a main-walk candidate gathered by the entity filter (kind 0 a map piece item, 1 an assembled character part
// `part`, 2 a SprjModelDrawEntity; I-0105: 3 a model particle's draw, added after the walk), admitted after the walk; the admission sets `code` (0 admitted, 1 zero fade,
// 2 missing bindings, 3 instance streams, 4 missing geometry, 5 no meshes; the order of the 1068 part refusal codes).
struct NativeDeferredCandidate {NativeSelectionCandidate candidate;uint8_t kind{},code{};int32_t part{};};
// 1088 witness: which missing-binding exit refuses a character part (site 1 current inputs, 2 stream patch,
// 3 material, 4 layout, 5 stream slot), with its owner entity and part index.
void NoteNativeAsmMissingBinding(const NativeSelectionCandidate& c,int site,uintptr_t detail=0) noexcept {
    if(!c.neutral_instance_decal)return;static LONG64 counts[6]{};const auto n=InterlockedIncrement64(&counts[site]);
    if(n<=8 || n%600==0){uintptr_t objects{},table{},detail_model{};Read(c.fade_owner+0xB10,&objects,8);
        if(site==2 && detail){Read(detail,&table,8);Read(detail+8,&detail_model,8);}
        char row[420]{};sprintf_s(row,"{\"event\":\"native_asm_part_missing_binding\",\"site\":%d,\"count\":%lld,\"owner\":%llu,\"part_index\":%lld,\"detail\":%llu,\"detail_table_rva\":%lld,\"detail_model\":%llu,\"part_model\":%llu}\n",
            site,n,static_cast<unsigned long long>(c.fade_owner),objects?static_cast<long long>((c.entity-objects)/0x108):-1ll,static_cast<unsigned long long>(detail),
            table?static_cast<long long>(table-image):-1ll,static_cast<unsigned long long>(detail_model),static_cast<unsigned long long>(c.model));Log(row);
        // 1176: which current-input check refuses a site-1 part (Load Game portraits lose their hair natively).
        if(site==1){uintptr_t model_table{},storage{},meshes{},begin{},end{},source{};int32_t count{};
            if(c.model){Read(c.model,&model_table,8);Read(c.model+0xE8,&storage,8);Read(c.model+0xA0,&count,4);Read(c.model+0xA8,&meshes,8);
                Read(c.model+0x1F8,&begin,8);Read(c.model+0x200,&end,8);}
            if(c.source_field)Read(c.source_field,&source,8);
            char more[400]{};sprintf_s(more,"{\"event\":\"native_asm_part_missing_inputs\",\"model\":%llu,\"model_table_rva\":%lld,\"storage\":%llu,\"count\":%d,\"meshes\":%llu,\"materials\":%lld,\"source_field\":%llu,\"source\":%llu,\"pose\":%llu,\"instance_field\":%llu}\n",
                static_cast<unsigned long long>(c.model),model_table?static_cast<long long>(model_table-image):-1ll,static_cast<unsigned long long>(storage),count,
                static_cast<unsigned long long>(meshes),end>=begin?static_cast<long long>((end-begin)/8):-1ll,static_cast<unsigned long long>(c.source_field),
                static_cast<unsigned long long>(source),static_cast<unsigned long long>(c.pose),static_cast<unsigned long long>(c.instance_field));Log(more);}}
}
// The model data name (+2B8, a wide string with its capacity at +2D0), printable ASCII only, for witnesses.
// Witness (thread « Sang sur persos, eau, siège », the Shrine Handmaid's white chair; test tooling, both renderers' prepare):
// hide-texture.txt next to the module holds a texture path fragment (case-insensitive); while the file is there (re-read every
// 2 s) the native mesh rows whose material names a texture containing it are not drawn, to pick a surface on screen by A/B.
// chr-positions.flag: each character's name and placement translation once per entity (native_chr_position), and the player's
// position every 300th schedule call (witness_player_position).
namespace chair_hunt {
std::mutex lock;std::wstring fragment;ULONGLONG checked{};LONG64 hidden_rows{};
std::mutex seen_lock;std::set<uintptr_t> seen;
std::wstring Fragment() noexcept {
    const auto now=GetTickCount64();const std::lock_guard hold(lock);
    if(checked && now<checked+2000)return fragment;checked=now;
    try{std::wstring next;
        if(const auto path=ModuleSibling(L"hide-texture.txt");!path.empty()){
            FILE* file{};if(!_wfopen_s(&file,path.c_str(),L"rb") && file){char text[128]{};const auto n=fread(text,1,sizeof(text)-1,file);fclose(file);
                for(size_t i=0;i<n;++i){const char ch=text[i];if(ch=='\r' || ch=='\n')break;next.push_back(static_cast<wchar_t>(tolower(static_cast<unsigned char>(ch))));}}}
        if(next!=fragment){char row[200]{};std::string ascii;for(const auto ch:next)ascii.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'/');
            sprintf_s(row,"{\"event\":\"native_hide_texture\",\"fragment\":\"%.120s\",\"hidden_rows\":%lld}\n",ascii.c_str(),hidden_rows);Log(row);}
        fragment=std::move(next);}catch(...){fragment.clear();}
    return fragment;
}
// "!name": nothing hidden; the draws of models whose name contains name are described once per (mesh, pass, recipe)
// (native_named_draw_sources: the programme family, the bound SRVs, b2 and b4 words).
// Several names may be given, separated by '|' (I-0113 gap 17: "!s040|s11600").
bool Matches(const std::wstring& part,uintptr_t model) noexcept {
    std::string name=NativeModelDataName(model);for(auto& ch:name)ch=static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    std::string wanted;
    for(size_t i=1;i<=part.size();++i){if(i<part.size() && part[i]!=L'|'){wanted.push_back(static_cast<char>(part[i]));continue;}
        if(!wanted.empty() && name.find(wanted)!=std::string::npos)return true;wanted.clear();}
    return false;
}
std::set<std::tuple<uintptr_t,uint32_t,uint32_t,uint32_t,int>> named_seen;   // I-0113 tranche 18: and b4's ditherFadeAlpha in twentieths (a fade's steps); tranche 21: and the mesh (meshes sharing a material were one row)
// "#name": the rows of models whose name contains name (case-insensitive) are not drawn.
bool NamedModel(const std::wstring& part,uintptr_t model) noexcept {
    std::string name=NativeModelDataName(model);for(auto& ch:name)ch=static_cast<char>(tolower(static_cast<unsigned char>(ch)));
    std::string wanted;for(size_t i=1;i<part.size();++i)wanted.push_back(static_cast<char>(part[i]));
    if(wanted.empty() || name.find(wanted)==std::string::npos)return false;InterlockedIncrement64(&hidden_rows);return true;
}
// A texture path of the mesh holds part (lower case).
bool HasTexture(const std::vector<ds3content::ModelTextureInput>& inputs,const std::wstring& part) noexcept {
    for(const auto& input:inputs){std::wstring path=input.path;for(auto& ch:path)ch=static_cast<wchar_t>(towlower(ch));
        if(path.find(part)!=std::wstring::npos)return true;}
    return false;
}
// "@x y z r [fragment]": nothing hidden; each (model, mesh) whose placement translation lies within r of (x, y z), and with a
// fragment one of whose texture paths holds it (I-0113 tranche 6: a far-placed map piece), is named once (native_model_near:
// model, translation, mesh, material, MTD, texture paths).
std::set<std::pair<uintptr_t,size_t>> near_seen;
template<class Material> void Near(const std::wstring& part,uintptr_t model,const std::array<float,12>& placement,const Material* material,size_t mesh) noexcept {
    float f[4]{};int used{};if(swscanf_s(part.c_str()+1,L"%f %f %f %f%n",&f[0],&f[1],&f[2],&f[3],&used)!=4)return;
    const float dx=placement[3]-f[0],dy=placement[7]-f[1],dz=placement[11]-f[2];if(dx*dx+dy*dy+dz*dz>f[3]*f[3])return;
    const auto filter=part.find_first_not_of(L' ',1+static_cast<size_t>(used));
    if(filter!=std::wstring::npos && !(material && HasTexture(material->texture_inputs,part.substr(filter,part.find_last_not_of(L' ')+1-filter))))return;
    // 300 rows without a fragment (as before), 600 with one (its models lie farther away).
    {const std::lock_guard hold(seen_lock);if(near_seen.size()>=(filter!=std::wstring::npos?600u:300u) || !near_seen.emplace(model,mesh).second)return;}
    const auto ascii=[](const std::wstring& t){std::string o;for(const auto c:t)if(o.size()<120)o.push_back(c>=32 && c<127 && c!='"' && c!='\\'?static_cast<char>(c):'/');return o;};
    std::string paths;if(material)for(const auto& t:material->texture_inputs){if(paths.size()>700)break;const auto cut=t.path.find_last_of(L"\\/");
        paths+=(paths.empty()?"":",")+ascii(cut==std::wstring::npos?t.path:t.path.substr(cut+1));}
    char row[1200]{};sprintf_s(row,"{\"event\":\"native_model_near\",\"model\":\"%.60s\",\"at\":[%.2f,%.2f,%.2f],\"mesh\":%zu,\"material\":\"%s\",\"mtd\":\"%s\",\"textures\":\"%s\"}\n",
        NativeModelDataName(model).c_str(),placement[3],placement[7],placement[11],mesh,material?ascii(material->name).c_str():"",
        material?ascii(material->mtd).c_str():"",paths.c_str());Log(row);
}
bool Hidden(const std::vector<ds3content::ModelTextureInput>& inputs,const std::wstring& part) noexcept {
    if(!HasTexture(inputs,part))return false;InterlockedIncrement64(&hidden_rows);return true;
}
}
std::string NativeModelDataName(uintptr_t model) {
    std::string name;uint64_t capacity{};uintptr_t text=model+0x2B8;Read(model+0x2D0,&capacity,8);if(capacity>7)Read(model+0x2B8,&text,8);
    for(int i=0;i<40;++i){wchar_t ch{};if(!Read(text+2ull*i,&ch,2) || !ch)break;name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');}
    return name;
}
// Workstream K P1-P3: the same blocks from the programme's plans (PlanNativeModelBlocks). Only the packs of the current
// ranges, the 1111 coverage of the mesh's numeric writes and the instance ranges remain per mesh, and each pack is shared:
// P2 scene and view blocks once per programme bytes for the scene (scene tables), instance blocks once per programme bytes
// and fade for the model (instance table, one AdmitNativeModelCandidate call), P3 numeric blocks once per programme and
// exact numeric writes (Programs::numeric_cache).
// (SharedRegisterBlock and NativeInstanceBlockTable are declared before NativeSelectedModel.)
// The key of one recipe and stage's programme: its bytes object (stable while the owning Programs lives), stage in bit 0.
uintptr_t NativeProgramKey(const NativeModelLayout::Programs& programs,size_t recipe,size_t stage) {
    return reinterpret_cast<uintptr_t>(&programs.ProgramBytes(recipe,stage))|stage;}
// A row's instance block (b4) of a stage: its model's table entry for the programme and fade.
// I-0074: looked up once when the pass row is made (PreparedMesh::instances), not at each prepare.
const SharedRegisterBlock& NativeMeshInstanceBlock(const NativeSelectedModel& model,const NativeSelectedMesh& mesh,uint32_t fade,size_t recipe,size_t stage) {
    static const SharedRegisterBlock none;
    if(fade>1)return none;
    if(!mesh.programs || !mesh.programs->interfaces[recipe][stage] || !model.instance_blocks)return none;
    const auto found=model.instance_blocks->find({NativeProgramKey(*mesh.programs,recipe,stage),fade});
    return found!=model.instance_blocks->end()?found->second:none;
}

bool EqualNumericWrites(const std::vector<ds3content::ModelParameterWrite>& a,const std::vector<ds3content::ModelParameterWrite>& b) {
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(a[i].byte_offset!=b[i].byte_offset || a[i].bytes!=b[i].bytes ||
        std::memcmp(a[i].words.data(),b[i].words.data(),a[i].bytes))return false;
    return true;
}
// C7-1b (PERSISTENT-RECORDS-DESIGN 1, MeshBlocks): the scene and view entries are a function of the programme and the
// scene, the instance entries of the programme and the model; since C7-1 no mesh holds them, so each programme fills them
// once per scene and once per model. C7-2: a model reused from its record calls this alone for each mesh's programme.
void PlanNativeProgramTables(const std::shared_ptr<const NativeModelLayout::Programs>& owner,const NativeSelectedModel& selected,
    NativeSelectedScene& scene,NativeInstanceBlockTable& instance_table,
    NativePlannedPrograms& planned_instances,
    std::span<const ds3content::ShaderConstantRange> scene_ranges,const ds3content::ShaderConstantRange& view_range) {
    // I-0071 point 3: the scene's tables are shared by the admission workers (scene.tables_lock); each thread remembers the
    // programmes it already found planned in this scene, so the lock is taken once per programme and thread.
    // I-0078 (i74-native-75: the reused meshes' table plans were a ninth of their admission): an open-addressed set of the owners
    // (a few hundred a scene), not a node set.
    struct Seen {const NativeSelectedScene* scene{};uint64_t uid{};std::vector<const void*> slots=std::vector<const void*>(1024);size_t count{};
        bool insert(const void* key){for(;;){const size_t mask=slots.size()-1;
                for(size_t at=(reinterpret_cast<uintptr_t>(key)*0x9E3779B97F4A7C15ull>>40)&mask;;at=(at+1)&mask){
                    if(slots[at]==key)return false;
                    if(!slots[at]){if(2*(count+1)>slots.size())break;slots[at]=key;++count;return true;}}
                std::vector<const void*> old(slots.size()*2);old.swap(slots);count=0;for(const auto k:old)if(k)insert(k);}}
        void clear(){std::fill(slots.begin(),slots.end(),nullptr);count=0;}};
    thread_local Seen seen;   // review L1: keyed by the scene uid
    if(seen.scene!=&scene || seen.uid!=scene.uid){seen.scene=&scene;seen.uid=scene.uid;seen.clear();}
    // I-0097 tranche 2 (i97-prof-3: at the area crossing the workers queued on tables_lock): one thread of the scene claims the
    // programme by its uid and alone asks the tables; the claim only filters (another scene's claim in between makes this one
    // ask again, and the tables answer).
    const auto claim=[&]{uint64_t prior=owner->planned_scene.load(std::memory_order_relaxed);
        while(prior!=scene.uid)if(owner->planned_scene.compare_exchange_weak(prior,scene.uid,std::memory_order_acq_rel))return true;
        return false;};
    std::unique_lock<std::mutex> tables;bool scene_new=false;
    if(seen.insert(owner.get()) && claim()){tables=std::unique_lock(scene.tables_lock);
        {const auto [entry,fresh]=scene.planned_programs.try_emplace(owner.get());if(fresh)*entry=owner;scene_new=fresh;}
        if(!scene_new)tables.unlock();}
    const bool instance_new=planned_instances.try_emplace(owner.get(),owner).second;
    if(!scene_new && !instance_new)return;
    const auto& programs=*owner;
    // I-0078 (i78-motion-02: while the camera moves no scene table is carried, about 1,400 scene and view blocks were packed a
    // collect while their thread held tables_lock, the workers waiting on it): the new programme's entries are made under the
    // lock, then packed after it; this thread alone planned the programme in this scene, so nothing else writes or reads them
    // before the scene is published (the tables' nodes stay where they are while other keys are added).
    if(scene_new){struct Pending {NativeSelectedScene::SharedBlock* entry;const ds3content::ShaderInterface* shader;const ds3content::ShaderRegisterPlan* plan;bool view;};
        std::array<Pending,native_recipe_count*3> pending{};size_t count{};
        for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage){
            const auto& shader=programs.interfaces[recipe][stage];if(!shader)continue;
            const auto& plan=programs.PlanOf(recipe,stage);const auto key=reinterpret_cast<const void*>(NativeProgramKey(programs,recipe,stage));
            if(const auto [entry,fresh]=scene.scene_block_table.try_emplace(key);fresh){entry->owner=owner;pending[count++]={entry,&*shader,&plan.scene,false};}
            if(!stage)if(const auto [entry,fresh]=scene.view_block_table.try_emplace(key);fresh){entry->owner=owner;pending[count++]={entry,&*shader,&plan.view,true};}}
        tables.unlock();
        for(size_t i=0;i<count;++i){const auto& p=pending[i];kprofile::Add(kprofile::SharedBlocksBuilt);
            const auto ranges=p.view?std::span<const ds3content::ShaderConstantRange>(&view_range,1):scene_ranges;
            p.entry->block=std::make_shared<const ds3content::ShaderRegisterBlock>(ds3content::PackShaderRegisterPlan(*p.shader,*p.plan,ranges));}}
    std::vector<ds3content::ShaderConstantRange> instance_ranges;
    static const std::array<uint8_t,65536> instance_zero{};
    // The instance records' word ranges, produced at first use (as before: never for a mesh without an interface).
    std::array<std::optional<std::vector<ds3content::ShaderConstantRange>>,2> instance_words;
    for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage){
        const auto& shader=programs.interfaces[recipe][stage];if(!shader)continue;kprofile::Add(kprofile::RecipeStagePairs);
        const auto& plan=programs.PlanOf(recipe,stage);
        const auto key=NativeProgramKey(programs,recipe,stage);
        for(uint32_t fade=0;instance_new && fade<selected.geometry_fades.count;++fade){
            auto& block=instance_table[{key,fade}];
            if(!block){
                auto& words=instance_words[fade];
                if(!words)words=NativeShaderWordRanges(selected.instances[fade].bytes,selected.instances[fade].written);
                instance_ranges.assign(words->begin(),words->end());
                if(plan.instance_tail)instance_ranges.push_back({ds3prod::instance_record_bytes,
                    std::span<const uint8_t>(instance_zero.data(),plan.instance_tail)});
                block=std::make_shared<const ds3content::ShaderRegisterBlock>(ds3content::PackShaderRegisterPlan(*shader,plan.instance,instance_ranges));
                kprofile::Add(kprofile::SharedBlocksBuilt);
            }
        }
    }
}
void PlannedNativeMeshBlocks(NativeSelectedMesh& bound,const NativeSelectedModel& selected,NativeSelectedScene& scene,
    NativeInstanceBlockTable& instance_table,NativePlannedPrograms& planned_instances,
    std::span<const ds3content::ShaderConstantRange> scene_ranges,const ds3content::ShaderConstantRange& view_range) {
    const auto& programs=*bound.programs;
    // P3: a write covers at most its four words; a longer one (none is produced) is never cached.
    const bool cacheable=std::all_of(bound.numeric_writes.begin(),bound.numeric_writes.end(),
        [](const auto& write){return write.bytes<=sizeof(write.words);});
    bool numeric_cached=false;
    if(cacheable){const std::lock_guard hold(programs.numeric_lock);
        for(auto& entry:programs.numeric_cache)if(EqualNumericWrites(entry.writes,bound.numeric_writes)){
            entry.used=++programs.numeric_clock;bound.numeric_blocks=entry.blocks;numeric_cached=true;break;}}
    if(!numeric_cached){kprofile::Add(kprofile::NumericBlockMisses);
        std::vector<ds3content::ShaderConstantRange> ranges,stage_ranges;
        for(const auto& write:bound.numeric_writes)ranges.push_back({write.byte_offset,
            {reinterpret_cast<const uint8_t*>(write.words.data()),write.bytes}});
        const auto covered=[&](uint32_t at){return std::any_of(bound.numeric_writes.begin(),bound.numeric_writes.end(),
            [&](const auto& write){return at>=write.byte_offset && at+4<=uint64_t(write.byte_offset)+write.bytes;});};
        static const uint32_t zero_words[16]{};
        for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage){
            const auto& shader=programs.interfaces[recipe][stage];if(!shader)continue;kprofile::Add(kprofile::RecipeStagePairs);
            const auto& plan=programs.PlanOf(recipe,stage);
            if(plan.composed_numeric){bound.numeric_blocks[recipe][stage]=plan.composed_numeric;continue;}
            stage_ranges.assign(ranges.begin(),ranges.end());
            for(const auto& variable:plan.mtd_variables){
                bool partial=false;for(uint32_t at=variable.offset;at<variable.offset+variable.bytes && at-variable.offset<64;at+=4)partial|=covered(at);
                if(partial)for(uint32_t at=variable.offset;at<variable.offset+variable.bytes && at-variable.offset<64;at+=4)
                    if(!covered(at))stage_ranges.push_back({at,{reinterpret_cast<const uint8_t*>(zero_words),4}});
                if(!partial && variable.fallback)for(const auto& [at,word]:variable.words)
                    stage_ranges.push_back({at,{reinterpret_cast<const uint8_t*>(&word),4}});
            }
            bound.numeric_blocks[recipe][stage]=std::make_shared<const ds3content::ShaderRegisterBlock>(
                ds3content::PackShaderRegisterPlan(*shader,plan.numeric,stage_ranges));
        }
    }
    // C7-1b: a mesh whose numeric blocks were found and whose programme is planned does no recipe-stage loop at all.
    PlanNativeProgramTables(bound.programs,selected,scene,instance_table,planned_instances,scene_ranges,view_range);
    if(cacheable && !numeric_cached){const std::lock_guard hold(programs.numeric_lock);
        auto& cache=programs.numeric_cache;
        if(cache.size()>=32)cache.erase(std::min_element(cache.begin(),cache.end(),[](const auto& a,const auto& b){return a.used<b.used;}));
        cache.push_back({bound.numeric_writes,bound.numeric_blocks,++programs.numeric_clock});}
}
// The collect's texture admission of one mesh: material texture bindings (FaceGen slot, genuine D24490 lookup with its
// "_l"/"_atlas000" packages, SYSTEX fallbacks), scene texture bindings, and the shader texture selection of every recipe
// and stage with its record checks: the selection witnesses and the checks.
void WitnessNativeShaderTextureSelection(const NativeSelectedMesh& bound,const NativeSelectedModel& selected,
    std::span<const ds3content::ModelSceneTextureBinding> scene_slots,size_t recipe,size_t stage,
    const ds3content::ShaderInterface& shader,ds3content::ModelTextureStatus status) {
    // 1203 witness: the resource outside the material texture envelope (RecursiveCrystal, Native1202).
    if(status==ds3content::ModelTextureStatus::Unsupported)if(const auto* odd=ds3content::UnsupportedModelShaderTexture(shader)){
        static LONG64 odd_samples{};if(InterlockedIncrement64(&odd_samples)<=16){
            std::string family;for(const auto ch:bound.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
            char row[512]{};sprintf_s(row,"{\"event\":\"native_texture_envelope_refused\",\"recipe\":%zu,\"stage\":%zu,\"name\":\"%.80s\",\"input_type\":%u,\"dimension\":%u,\"return_type\":%u,\"count\":%u,\"register\":%u,\"family\":\"%.120s\"}\n",
                recipe,stage,odd->name.c_str(),odd->input_type,odd->dimension,odd->return_type,odd->count,odd->shader_register,family.c_str());Log(row);}}
    if(status==ds3content::ModelTextureStatus::Missing){
        // 1035 witness: a shader texture register without any binding.
        static LONG64 unbound_samples{},offscreen_unbound_samples{};
        if(InterlockedIncrement64(native_offscreen_preparing?&offscreen_unbound_samples:&unbound_samples)<=16)for(const auto& resource:shader.resources){
            if(resource.input_type!=2)continue;
            const bool bound_material=std::any_of(bound.programs->texture_bindings.begin(),bound.programs->texture_bindings.end(),
                [&](const auto& row){return row.declaration.slot==resource.shader_register;});
            const bool bound_scene=std::any_of(scene_slots.begin(),scene_slots.end(),[&](const auto& row){return row.slot==resource.shader_register;});
            if(bound_material || bound_scene)continue;
            std::string material_inputs;
            for(const auto& input:bound.material->texture_inputs){if(material_inputs.size()>600)break;material_inputs+=material_inputs.empty()?"":",";
                for(const auto ch:input.type)material_inputs.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                material_inputs+=input.path.empty()?"(empty)":"";}
            uintptr_t patch_table{};if(selected.patch)Read(selected.patch,&patch_table,8);
            char row[1100]{};sprintf_s(row,"{\"event\":\"native_selected_texture_unbound\",\"offscreen\":%s,\"slot\":%u,\"name\":\"%.80s\",\"decal_choice\":%d,\"patch_table_rva\":%llu,\"material_inputs\":\"%.700s\"}\n",
                native_offscreen_preparing?"true":"false",resource.shader_register,resource.name.c_str(),selected.decal_choice?static_cast<int>(*selected.decal_choice):-1,
                static_cast<unsigned long long>(patch_table>image?patch_table-image:0),material_inputs.c_str());Log(row);break;}
    }
}
// A Ready selection is downgraded per mesh when a bound record is absent or its shape does not match the resource.
void CheckNativeShaderTextureRecords(const NativeSelectedMesh& bound,const ds3content::ShaderInterface& shader,
    NativeSelectedMesh::ShaderTextures& output) {
    if(output.status!=ds3content::ModelTextureStatus::Ready)return;
    for(const auto& request:output.registers){
        const auto texture=request.source==ds3content::ModelTextureSource::Scene?bound.scene_texture_bindings.at(request.binding).texture:
            bound.texture_bindings[request.binding].texture;
        if(!texture){output.status=ds3content::ModelTextureStatus::Missing;
            // 1035 witness: the bound register whose named texture is absent.
            static LONG64 absent_samples{};
            if(request.source==ds3content::ModelTextureSource::Material && InterlockedIncrement64(&absent_samples)<=16){
                const auto& binding=bound.programs->texture_bindings[request.binding];std::string path,type;
                for(const auto ch:binding.path)path.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'/');
                for(const auto ch:binding.declaration.name)type.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                char row[640]{};sprintf_s(row,"{\"event\":\"native_selected_texture_absent\",\"slot\":%u,\"kind\":%u,\"type\":\"%.80s\",\"path\":\"%.300s\"}\n",
                    request.slot,binding.declaration.kind,type.c_str(),path.c_str());Log(row);}
            break;}
        // The bound record's shape must match the shader resource: 2D (4),
        // cube (9, 1114 IBL) or a structured buffer (input 5, 1114 lights).
        const auto declared=std::find_if(shader.resources.begin(),shader.resources.end(),
            [&](const auto& resource){return resource.shader_register==request.slot && resource.input_type!=0 && resource.input_type!=3;});
        const bool buffer=declared!=shader.resources.end() && declared->input_type==5;
        const bool cube=declared!=shader.resources.end() && declared->dimension==9;
        if(declared==shader.resources.end() ||
           (buffer?(texture->description.format!=DXGI_FORMAT_UNKNOWN || texture->description.cube):
            cube?(!texture->description.cube || texture->description.slices!=6):
            (texture->description.cube || texture->description.slices!=1 || texture->description.format==DXGI_FORMAT_UNKNOWN))){
            static LONG64 shape_samples{};if(InterlockedIncrement64(&shape_samples)<=16){char row[256]{};
                sprintf_s(row,"{\"event\":\"native_texture_shape_refused\",\"slot\":%u,\"scene\":%s,\"format\":%d,\"slices\":%u,\"cube\":%s,\"declared_dimension\":%u}\n",
                    request.slot,request.source==ds3content::ModelTextureSource::Scene?"true":"false",static_cast<int>(texture->description.format),
                    texture->description.slices,texture->description.cube?"true":"false",declared!=shader.resources.end()?declared->dimension:0u);Log(row);}
            output.status=ds3content::ModelTextureStatus::Unsupported;break;}
    }
}
// 1036 witness: why a named material texture stays unbound (the last candidate's holder, table and texture).
void WitnessNativeTextureUnbound(const ds3content::ModelTextureBinding& input,uintptr_t holder,uintptr_t table,uintptr_t texture) {
    static LONG64 lookup_samples{};
    const auto lookup_sample=InterlockedIncrement64(&lookup_samples);if(lookup_sample<=16 || lookup_sample%20000==0){
        const auto record=texture?textures.Acquire(texture):nullptr;std::string path;
        for(const auto ch:input.path)path.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'/');
        char row[640]{};sprintf_s(row,"{\"event\":\"native_texture_lookup_unbound\",\"holder\":%s,\"holder_table_rva\":%llu,\"texture\":%s,\"record\":%s,\"resident\":%s,\"view\":%s,\"path\":\"%.300s\"}\n",
            holder?"true":"false",static_cast<unsigned long long>(table && table>image?table-image:0),texture?"true":"false",
            record?"true":"false",record && record->resident?"true":"false",record && record->view?"true":"false",path.c_str());Log(row);}
}
// Lightmap floors (WAVE1-LAST-EFFECTS-STATIC-PREP 1.5): the outcome of the DOL lightmap lookups (t26 g_DOLTexture, t27
// g_DOLTexture2): the path itself, its "_l" low-resolution package, its "_atlas000" package, nothing found, or the loader's
// SYSTEX_WHITE default (chosen ~1). Counts per slot every 4,000th lookup; the first 12 lookups that did not bind the path, named.
void WitnessNativeLightmapLookup(const ds3content::ModelTextureBinding& input,size_t chosen,bool bound,uintptr_t resolver,
    std::span<const std::wstring> candidates) {
    static LONG64 counts[2][5]{};static LONG64 lookups{},named{};
    const unsigned slot=input.declaration.slot==27?1u:0u;
    const unsigned outcome=chosen==~size_t{1}?4u:!bound?3u:chosen<3?static_cast<unsigned>(chosen):3u;
    InterlockedIncrement64(&counts[slot][outcome]);
    // I-0100 tranche 2 (Anor Londo, a death's reload: the lookups that bind nothing rise from 1 % to 70 %): the first 12 named
    // again after each scene reset (a load), with popin-observe.flag only (12 a process otherwise).
    if(PopinObserve()){static std::atomic<uint64_t> named_resets{};
        if(const auto resets=NativeKeptDrawResets();named_resets.load(std::memory_order_relaxed)!=resets && named_resets.exchange(resets)!=resets)
            InterlockedExchange64(&named,0);}
    if(outcome==3 && InterlockedIncrement64(&named)<=12){
        // Each candidate's genuine D24490 holder, its table, its texture word and the native ingress record of that texture.
        std::string cells;
        for(const auto& candidate:candidates){uintptr_t holder{},table{},texture{};
            bool busy{};holder=NativeTextureRegistryFind(resolver,candidate.c_str(),busy);
            if(holder){Read(holder,&table,8);Read(holder+0x10,&texture,8);}
            const auto record=texture?textures.Acquire(texture):nullptr;std::string name;
            for(const auto ch:candidate)name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'/');
            const auto slash=name.find_last_of('/');if(slash!=std::string::npos)name.erase(0,slash+1);
            char cell[400]{};sprintf_s(cell,"%s{\"name\":\"%.120s\",\"holder\":%s,\"table_rva\":%llu,\"texture\":%s,\"record\":%s,\"resident\":%s}",cells.empty()?"":",",name.c_str(),
                holder?"true":"false",static_cast<unsigned long long>(table>image?table-image:0),texture?"true":"false",record?"true":"false",record && record->resident?"true":"false");cells+=cell;}
        std::string path;for(const auto ch:input.path)path.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'/');
        const std::string row="{\"event\":\"native_lightmap_lookup\",\"slot\":"+std::to_string(input.declaration.slot)+",\"path\":\""+path.substr(0,300)+"\",\"candidates\":["+cells+"]}\n";Log(row.c_str());}
    if(const auto n=InterlockedIncrement64(&lookups);n==200 || n%4000==0){char row[400]{};
        sprintf_s(row,"{\"event\":\"native_lightmap_lookups\",\"lookups\":%lld,\"t26\":{\"path\":%lld,\"low\":%lld,\"atlas\":%lld,\"none\":%lld,\"white\":%lld},"
            "\"t27\":{\"path\":%lld,\"low\":%lld,\"atlas\":%lld,\"none\":%lld,\"white\":%lld}}\n",n,counts[0][0],counts[0][1],counts[0][2],counts[0][3],counts[0][4],
            counts[1][0],counts[1][1],counts[1][2],counts[1][3],counts[1][4]);Log(row);}
}
// Lightmap floors (Reference1851, K-0151, K-0153): the original binds a lightmapped draw's lightmaps in its ForLightMap draw (1A9C30
// calls slot 1 of the draw's GXFlverModelDrawInstanceParam, 19CA80), not from the material's texture paths, which resolve to
// SYSTEX_DUMMY (white). With the draw parameter bit 13 (+0x18C) clear and the param's lightmap set P (+0x30), 5B110 binds P[3], P[4]
// at t26/t27 (a DOL set) or P[0..2] at t26..t28, each object's slot +0x10, and the pixel constant buffer 0 (cbTexHdrScaleParam)
// from P[3]'s slot +0x70 (the texture's authored constant, 147090/1471E0), else the default [root+0x46E0]. The param is the +8 of
// the object 57A20 receives (K-0154). Natively: the same objects' native textures and constants. OPEN: bit 13's producer (the
// rows the native draws are G-buffer and forward rows, where it is taken clear) and 19CA80's +0x38 default set (the GX default
// textures, kept as the material's binding).
struct NativeLightmapSet {uintptr_t set{};std::array<uintptr_t,5> objects{};};
// The set is the draw descriptor's +0x90 (Reference1856 at m30: the set 19CA80 binds is the map piece item's +0x100, its descriptor
// +0x70 plus 0x90, and the item's +8 is the param; the emit copies the address to the param's +0x30, which the native draw does not
// run); 5B3D0 reads the same descriptor's +0xB8 (atlasUvScaleOffset). The descriptor is the candidate's (D3DA20 item+0x70, D079C0
// entity+0x3D0). No object in it: no set.
std::optional<NativeLightmapSet> ReadNativeLightmapSet(uintptr_t descriptor) {
    NativeLightmapSet out;
    if(!descriptor)return std::nullopt;
    out.set=descriptor+0x90;
    if(!Read(out.set,out.objects.data(),sizeof(out.objects)) || std::all_of(out.objects.begin(),out.objects.end(),[](uintptr_t object){return object==0;}))return std::nullopt;
    return out;
}// The native texture of one lightmap object of a set (a GXTexture2D the native ingress admitted; a SprjGxTexture through its +0x10).
uintptr_t NativeLightmapReceiver(uintptr_t object) {
    uintptr_t table{};
    if(object && Read(object,&table,8) && table==image+0x29559E8)return NativeHolderInner(object);   // I-0071: through the holder chain
    return object;
}
// I-0071: a set object that is a holder (SprjGxTexture) is bound as the first candidate of its slot, ahead of the material's
// own candidates, and rebound like them when its inner changes (a lightmap not yet resident binds the material's texture).
void ApplyNativeLightmapSet(NativeSelectedMesh& bound,const std::optional<NativeLightmapSet>& set) {
    bound.lightmap_constant.reset();
    if(!set)return;
    const bool dol=set->objects[3]!=0;
    bool applied=false,constant_slot=false;
    for(auto& binding:bound.texture_bindings){const auto slot=binding.input.declaration.slot;
        const int k=dol?(slot==26?3:slot==27?4:-1):(slot>=26 && slot<=28?static_cast<int>(slot)-26:-1);
        if(k<0 || !set->objects[k])continue;
        uintptr_t table{};
        if(!binding.set_first && Read(set->objects[k],&table,8) && table==image+0x29559E8){
            binding.holders={set->objects[k],binding.holders[0],binding.holders[1],binding.holders[2]};binding.set_first=true;binding.dol_constant=dol && k==3;size_t chosen=~size_t{};
            auto record=ResolveNativeTextureHolders(binding.holders,binding.inners,&chosen);
            if(record && chosen==0){binding.texture=std::move(record);applied=true;constant_slot|=binding.dol_constant;}}
        else if(auto record=textures.Acquire(set->objects[k]);record && record->resident && record->resource && record->view){
            binding.texture=std::move(record);applied=true;if(dol && k==3)constant_slot=true;}}
    if(constant_slot){const auto receiver=NativeLightmapReceiver(set->objects[3]);   // review: the block's sharing key is the t26 record
        const std::lock_guard hold(native_texture_constant_lock);
        if(const auto found=native_texture_constants.find(receiver);found!=native_texture_constants.end())bound.lightmap_constant=found->second;}
    // Witness: the first sets met, their objects' classes and native records.
    static LONG64 rows{};if(InterlockedIncrement64(&rows)<=8){std::string cells;
        for(const auto object:set->objects){uintptr_t table{};if(object)Read(object,&table,8);const auto record=object?textures.Acquire(NativeLightmapReceiver(object)):nullptr;
            char cell[200]{};sprintf_s(cell,"%s{\"present\":%s,\"table_rva\":%llu,\"record\":%s,\"resident\":%s,\"width\":%u}",cells.empty()?"":",",object?"true":"false",
                static_cast<unsigned long long>(table>image?table-image:0),record?"true":"false",record && record->resident?"true":"false",record?record->description.width:0u);cells+=cell;}
        char head[200]{};sprintf_s(head,"{\"event\":\"native_lightmap_set\",\"dol\":%s,\"applied\":%s,\"constant\":[%g,%g,%g,%g],\"objects\":[",dol?"true":"false",applied?"true":"false",
            bound.lightmap_constant?(*bound.lightmap_constant)[0]:-1.f,bound.lightmap_constant?(*bound.lightmap_constant)[1]:-1.f,
            bound.lightmap_constant?(*bound.lightmap_constant)[2]:-1.f,bound.lightmap_constant?(*bound.lightmap_constant)[3]:-1.f);
        const std::string text=head+cells+"]}\n";Log(text.c_str());}
}
// WATER-TESS-WAVE1-READY WT-2: the readable depth's record (t56), made once per resource of the renderer.
std::shared_ptr<const bootstrap::NativeTextureRecord> NativeDecalSourceRecord(const Microsoft::WRL::ComPtr<ID3D12Resource>& resource,DXGI_FORMAT view_format,const char* what);
std::mutex native_water_lock;std::shared_ptr<const bootstrap::NativeTextureRecord> native_water_depth;
// WATER-TESS-WAVE1-READY WT-6 (on by default since D-0189, no-water-reflection.flag opts out), I-0125 R6c and R6f (D-0251: the
// mirror view removed): the water's g_ReflectionTexture (t54), an RGBA16F target of half the output extent that the main frame's
// traced pass writes from the water's own surface (the plane of the nearest water mesh, the box centre, n = +Y: WT-6).
struct NativeReflectionState {
    std::mutex lock;
    Microsoft::WRL::ComPtr<ID3D12Resource> target;std::shared_ptr<const bootstrap::NativeTextureRecord> record;
    uint32_t width{},height{};
    bool written{};   // the traced pass has recorded into the target (before that its contents are undefined: t54 stays black)
    // The latest collect chose the traced water (CollectNativeWaterPlane): the target for the main frame's pass (its plane in the
    // scene: rt_water_plane), which marks it written (RtWaterDrawn).
    bool traced{};
};
NativeReflectionState native_reflection;
std::atomic<uint64_t> native_rt_water_planes{};   // R6c: the collects that published a plane for the traced water (native_rt_water)
// D-0189 (Thomas, 2026-10-18 21:58): on by default, as the original always has it; no-water-reflection.flag opts out.
bool NativeReflectionEnabled() noexcept {static const bool on=!ModuleFlag(L"no-water-reflection.flag");return on;}
// The record while the latest collect chose the traced water and the target holds a picture; null otherwise (the water binds the
// black default, as the original does without a reflection object).
std::shared_ptr<const bootstrap::NativeTextureRecord> NativeReflectionRecord() {
    if(!NativeReflectionEnabled())return {};const std::lock_guard hold(native_reflection.lock);
    return native_reflection.traced && native_reflection.written?native_reflection.record:nullptr;}
std::shared_ptr<const bootstrap::NativeTextureRecord> NativeWaterDepthRecord() {
    const auto resource=renderer.WaterDepthCopy();if(!resource)return {};
    const std::lock_guard hold(native_water_lock);
    if(!native_water_depth || native_water_depth->resource.Get()!=resource.Get())
        native_water_depth=NativeDecalSourceRecord(resource,DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,"native_water_depth_view");
    return native_water_depth;}
// WATER-TESS-WAVE1-READY WT-2 (FUN_140103510's binds, 1.3): a water material's t54 g_ReflectionTexture (the black default until the
// traced reflection, I-0125 R6c), t55 g_RefractionTexture (the frame copy), t56 g_DepthTexture (the readable depth copy) and t57
// g_WaterInteractTexture (the grey default: no interaction map, the neutral 0.502).
template<class Admit> void AdmitNativeWaterSlots(const NativeModelLayout::Programs* programs,const NativeCommonImages& common,
    const std::shared_ptr<const bootstrap::NativeTextureRecord>& copy,Admit&& admit) {   // I-0097 tranche 2: the scene's copy
    if(!programs || !programs->policy || !(programs->policy->flags&0x30))return;
    const auto named=[&](const wchar_t* name)->std::shared_ptr<const bootstrap::NativeTextureRecord>{
        for(const auto& [system_name,texture]:common.system)if(texture && _wcsicmp(system_name.c_str(),name)==0)return texture;return {};};
    const auto reflection=NativeReflectionRecord();   // I-0125 R6c: the traced water's target once written, else the black default
    admit(54,reflection?reflection:named(L"SYSTEX_BLACK"));admit(55,copy);admit(56,NativeWaterDepthRecord());admit(57,named(L"SYSTEX_GRAY"));}
// DECALS-PAINT-READY rung 1: the views of the receiver of a GXBloodyMaterialPatch (+8), once its target is made.
NativePaintViews NativePaintViewsOf(uintptr_t patch) {
    uintptr_t receiver{};if(!patch || !Read(patch+8,&receiver,8) || !receiver)return {};
    const std::lock_guard hold(native_paint_lock);const auto found=native_paints.find(receiver);
    return found==native_paints.end()?NativePaintViews{}:NativePaintViews{found->second.diffuse,found->second.specular};}
// A material binding none of whose holders holds a resident texture: the system texture of its stem (SYSTEX_*), else the
// loader's SYSTEX_DUMMY = SYSTEX_WHITE for a texture bound with no data (1115), else none (the row is then incomplete).
std::shared_ptr<const bootstrap::NativeTextureRecord> NativeFallbackTexture(const ds3content::ModelTextureBinding& input,
    const NativeModelLayout::Programs::TextureLookup& lookup,const NativeSelectedScene::SystemTextures& system,bool common,
    const NativeSelectedMesh& bound,uintptr_t model,uintptr_t resolver,uintptr_t holder,uintptr_t inner) {
    if(common)if(const auto found=system.by_name.find(lookup.leaf_folded);found!=system.by_name.end())return found->second;
    if(common && NativeDefaultBoundTextureFolded(lookup.leaf_folded)){
        if(input.declaration.slot==26 || input.declaration.slot==27)WitnessNativeLightmapLookup(input,~size_t{1},true,resolver,lookup.candidates);
        // Witness (the Shrine Handmaid's white chair): a material texture that falls back to SYSTEX_WHITE, once per path.
        {static std::mutex white_lock;static std::set<std::wstring> white_seen;bool fresh{};
            {const std::lock_guard hold(white_lock);fresh=white_seen.size()<96 && white_seen.insert(input.path).second;}
            if(fresh){const auto ascii=[](const std::wstring& t){std::string o;for(const auto c:t)if(o.size()<150)o.push_back(c>=32 && c<127 && c!='"' && c!='\\'?static_cast<char>(c):'/');return o;};
                char row[640]{};sprintf_s(row,"{\"event\":\"native_texture_white_fallback\",\"slot\":%u,\"path\":\"%s\",\"model\":\"%.60s\",\"material\":\"%s\",\"mtd\":\"%s\"}\n",
                    input.declaration.slot,ascii(input.path).c_str(),NativeModelDataName(model).c_str(),
                    bound.material?ascii(bound.material->name).c_str():"",bound.material?ascii(bound.material->mtd).c_str():"");Log(row);}}
        return system.white_bound;}
    uintptr_t table{};if(holder)Read(holder,&table,8);
    WitnessNativeTextureUnbound(input,holder,table,inner);
    return {};
}
void SelectNativeMeshShaderTextures(NativeSelectedMesh& bound,const NativeSelectedModel* selected);
// Workstream K P4a/P5: the same admission from the programme's path-only plan (PlanNativeModelTextures), the scene's
// system textures resolved once, and the shader texture selection cached per programme and ordered scene slots (the
// genuine D24490 lookups, the record leases, the scene-copy and decal decisions and the record checks stay per mesh).
// The scene's system textures by name, resolved once per collect (I-0071: also for a kept mesh rebound to its fallback).
void EnsureNativeSystemTextures(NativeSelectedScene& result) {
    auto& system=result.system_textures;
    if(!system.resolved && result.common_images){system.resolved=true;
        // (step review: the main collect's own copy, read with its record epochs before the walk, when it has them)
        if(result.record_epochs)system.scene_copy=std::static_pointer_cast<const bootstrap::NativeTextureRecord>(result.record_epochs->scene_copy);   // stored from native_scene_copy
        else{const std::lock_guard hold(native_scene_copy_lock);system.scene_copy=native_scene_copy;}
        for(const auto& [system_name,system_texture]:result.common_images->system){
            if(!system.white_bound && system_texture && _wcsicmp(system_name.c_str(),L"SYSTEX_WHITE")==0)system.white_bound=system_texture;}
        for(const auto& [system_name,system_texture]:result.common_images->system)if(_wcsicmp(system_name.c_str(),L"SYSTEX_WHITE")==0){system.white_named=system_texture;break;}
        for(const auto& [system_name,system_texture]:result.common_images->system)if(_wcsicmp(system_name.c_str(),L"SYSTEX_ZERO")==0){system.zero_named=system_texture;break;}
        for(const auto& [system_name,system_texture]:result.common_images->system)if(system_texture){std::wstring folded(system_name);
            for(auto& ch:folded)ch=static_cast<wchar_t>(towlower(ch));system.by_name.try_emplace(std::move(folded),system_texture);}}
}
// I-0140 (the player's white face in cutscenes, I-0139 tranche 3): a FaceGen binding left without a composed face (the white
// texture stands in), named by why: no owner, an owner the native never initialised (not in face_images: its vtable says whose
// it is), or one not yet composed (+0x91, status +0x70). Behind face-unbound-observe.flag (its final review's L1): its first rows per
// owner and model, then sparse.
void WitnessNativeFaceUnbound(const NativeSelectionCandidate& c,uintptr_t model) {
    static const bool observe=ModuleFlag(L"face-unbound-observe.flag");if(!observe)return;
    const uintptr_t owner=c.face_owner;
    // Its step review (L4): counted first, by owner and model (a cutscene's player is a new owner after the tour's NPC faces; the
    // bindings without an owner told apart by their model, its final review's L1), then read.
    static std::mutex lock;static std::map<std::pair<uintptr_t,uintptr_t>,uint64_t> seen;uint64_t n{};
    {const std::lock_guard hold(lock);if(seen.size()<1024 || seen.contains({owner,model}))n=++seen[{owner,model}];}
    if(!n || (n>2 && n%2000))return;
    const char* reason="no_owner";uintptr_t table{};uint32_t status{};BYTE composed{};
    if(owner){Read(owner,&table,8);Read(owner+0x70,&status,4);Read(owner+0x91,&composed,1);
        const std::shared_lock hold(face_image_lock);const auto found=face_images.find(owner);
        reason=face_images_closed?"closed":found==face_images.end()?"not_initialised":!composed?"not_composed":!found->second.composed?"no_image":"unknown";}
    uintptr_t entity_table{};Read(c.entity,&entity_table,8);std::string name;for(const auto ch:NativeModelDataName(model))if(name.size()<60)name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
    char row[480]{};sprintf_s(row,"{\"event\":\"native_face_unbound\",\"reason\":\"%s\",\"model\":\"%s\",\"owner\":%llu,\"owner_table_rva\":%lld,\"status\":%u,\"composed\":%u,"
        "\"entity_table_rva\":%lld,\"draw_colour\":%s,\"count\":%llu}\n",reason,name.c_str(),static_cast<unsigned long long>(owner),
        table?static_cast<long long>(table-image):-1ll,status,composed,entity_table?static_cast<long long>(entity_table-image):-1ll,c.draw_colour?"true":"false",
        static_cast<unsigned long long>(n));Log(row);
}
void PlannedNativeMeshTextures(NativeSelectedMesh& bound,const NativeSelectedModel& selected,const NativeSelectionCandidate& c,
    NativeSelectedScene& result,const NativeModelLayout& layout) {
    EnsureNativeSystemTextures(result);auto& system=result.system_textures;
    const bool available=bound.programs && bound.programs->texture_bindings_available;
    if(available){
        const auto& programs=*bound.programs;
        bound.texture_bindings.reserve(programs.texture_bindings.size());
        for(size_t index=0;index<programs.texture_bindings.size();++index){
            const auto& input=programs.texture_bindings[index];const auto& lookup=programs.texture_lookups[index];
            NativeSelectedMesh::TextureBinding texture_binding;texture_binding.input=input;
            if(lookup.facegen){
                texture_binding.texture=NativeComposedFace(c.face_owner);
                if(!texture_binding.texture)WitnessNativeFaceUnbound(c,selected.model);
                if(!texture_binding.texture && result.common_images)texture_binding.texture=system.white_bound;
            }else{
                uintptr_t named_registry{};
                if(!input.path.empty() && Read(image+0x4792F58,&named_registry,8) && named_registry){
                    // I-0071: through the candidates' holders (ER B); what they hold is resolved now and again at each reuse.
                    bool busy{};texture_binding.holders=NativeTextureHolders(programs,index,layout.resolver,named_registry,&busy);texture_binding.looked_up=true;
                    // C1: a lookup given up leaves the binding unbound (no stand-in: a row that samples it is refused this frame) and
                    // the model unrecorded, so the next collect admits it afresh (its fallback then, if the path has no holder).
                    if(busy){bound.texture_gave_up=true;bound.texture_bindings.push_back(std::move(texture_binding));continue;}
                    size_t chosen=~size_t{};
                    texture_binding.texture=ResolveNativeTextureHolders(texture_binding.holders,texture_binding.inners,&chosen);
                    if(input.declaration.slot==26 || input.declaration.slot==27)WitnessNativeLightmapLookup(input,chosen,texture_binding.texture!=nullptr,layout.resolver,lookup.candidates);
                    if(!texture_binding.texture)texture_binding.texture=NativeFallbackTexture(input,lookup,system,result.common_images!=nullptr,bound,selected.model,layout.resolver,
                        texture_binding.holders[0],texture_binding.inners[0]);
                }
            }
            bound.texture_bindings.push_back(std::move(texture_binding));
        }
    }
    if(result.common_images){
        const auto admit_scene=[&](uint32_t slot,const std::shared_ptr<const bootstrap::NativeTextureRecord>& texture){
            if(texture && texture->resident && texture->resource && texture->view)
                bound.scene_texture_bindings.push_back({slot,texture});};
        // I-0113 tranche 22 (gap 21): t38 is FF630's 4x4 A8 ordered dither, not 7ECB0's 64x64 rotation noise (whose alpha is 0: every
        // dithered fade discarded all its pixels at once). The row's own choice of the pair is made where its textures resolve (13b).
        admit_scene(38,result.common_images->fade_dither[0]);
        if(system.scene_copy && !(available && bound.programs->names_slot45))admit_scene(45,system.scene_copy);
        AdmitNativeWaterSlots(bound.programs.get(),*result.common_images,system.scene_copy,admit_scene);   // WT-2
        // I-0105 (Lightning Spear's straight core, s03000 GXFlver_ColZFeather refused for its unbound t56): the original binds the
        // readable depth copy as t56 for the passes after it (FUN_140103510's common rows t54..t56), not only for water.
        if(bound.programs && bound.programs->samples_depth56 && !(bound.programs->policy && (bound.programs->policy->flags&0x30)) &&
           !(bound.programs->named_scene_slots>>(56-30)&1))admit_scene(56,NativeWaterDepthRecord());
        const bool bloody_provider=selected.decal_choice==ingress::ModelDecalChoice::InstanceProvider && selected.patch &&
            [&]{uintptr_t table{};return Read(selected.patch,&table,8) && table==image+0x3D41DF0;}();
        if(selected.decal_choice==ingress::ModelDecalChoice::Common || selected.decal_choice==ingress::ModelDecalChoice::None ||
           selected.neutral_instance_decal || bloody_provider){
            // DECALS-PAINT-READY rung 1: the receiver's paint target once it is made (its first paint), the neutral decal before.
            const auto paint=bloody_provider?NativePaintViewsOf(selected.patch):NativePaintViews{};
            admit_scene(41,paint.diffuse?paint.diffuse:result.common_images->absent_decal);admit_scene(42,paint.specular?paint.specular:result.common_images->absent_decal);
        }
        const auto unnamed=[&](uint32_t slot){return !bound.programs || !(bound.programs->named_scene_slots>>(slot-30)&1);};
        if(unnamed(30))admit_scene(30,result.common_images->neutral_ibl);
        if(unnamed(31))admit_scene(31,result.common_images->neutral_ibl);
        if(unnamed(33))admit_scene(33,system.white_named);
        if(unnamed(44))admit_scene(44,system.zero_named);
        // I-0105 tranche 10: the snow textures t47..t51 (103510's common rows) for a programme that reads them; an object without a
        // resident record leaves its slot unbound (the draw stays refused: no stand-in, D-0197).
        if(bound.programs && bound.programs->samples_snow && result.snow_textures)for(uint32_t i=0;i<5;++i)
            if(bound.programs->samples_snow>>i&1 && unnamed(47+i))admit_scene(47+i,result.snow_textures->records[i]);
        // I-0107: the main view's light grid (t59..t62, 1A3520) once the renderer made it; the empty buffers otherwise and for the
        // offscreen views (the grid is the main camera's).
        const auto* grid=result.record_epochs && !native_offscreen_preparing?
            static_cast<const NativeLightGridRecords*>(result.record_epochs->light_grid.get()):nullptr;
        for(uint32_t index=0;index<4;++index)if(unnamed(59+index))admit_scene(59+index,grid?grid->buffers[index]:result.common_images->empty_light_buffers[index]);
    }
    kprofile::Switch(kprofile::MeshShaderTex);
    if(available)SelectNativeMeshShaderTextures(bound,&selected);
}
// P5: the shader texture selection of a mesh (the programme's cache by ordered scene slots), then the record checks of its
// bindings; I-0071: also after a kept mesh is rebound (`selected` null: no selection witness).
void SelectNativeMeshShaderTextures(NativeSelectedMesh& bound,const NativeSelectedModel* selected) {
    const auto& programs=*bound.programs;
    std::vector<uint32_t> key;key.reserve(bound.scene_texture_bindings.size());
    for(const auto& scene_binding:bound.scene_texture_bindings)key.push_back(scene_binding.slot);
    bool cached=false;
    {const std::lock_guard hold(programs.shader_texture_lock);
        for(auto& entry:programs.shader_texture_cache)if(entry.scene_slots==key){entry.used=++programs.shader_texture_clock;
            for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage){
                if(!programs.interfaces[recipe][stage])continue;
                auto& output=bound.shader_textures[recipe][stage];
                output.status=entry.selections[recipe][stage].first;output.registers=entry.selections[recipe][stage].second;}
            cached=true;break;}}
    if(!cached){
        std::vector<ds3content::ModelSceneTextureBinding> scene_slots;
        for(const auto slot:key)scene_slots.push_back({slot});
        NativeModelLayout::Programs::ShaderTextureEntry entry;entry.scene_slots=key;
        for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage){
            auto& output=bound.shader_textures[recipe][stage];
            const auto& shader=programs.interfaces[recipe][stage];if(!shader)continue;
            output.status=ds3content::SelectModelShaderTextures(*shader,programs.texture_bindings,scene_slots,output.registers);
            if(selected)WitnessNativeShaderTextureSelection(bound,*selected,scene_slots,recipe,stage,*shader,output.status);
            entry.selections[recipe][stage]={output.status,output.registers};
        }
        kprofile::Add(kprofile::ShaderTextureMisses);
        // I-0105 tranche 10 witness: a snow programme's selection (each recipe and stage's status, 0 Ready) and its scene slots.
        if(selected && programs.samples_snow){static LONG64 snow_selections{};if(InterlockedIncrement64(&snow_selections)<=24){
            std::string row="{\"event\":\"native_snow_selection\",\"family\":\"";
            for(const auto ch:programs.program_path)if(row.size()<200)row.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
            row+="\",\"reads\":"+std::to_string(programs.samples_snow)+",\"status\":[";bool first=true;
            for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage)if(programs.interfaces[recipe][stage]){
                row+=(first?"":",")+std::to_string(static_cast<int>(entry.selections[recipe][stage].first));first=false;}
            row+="],\"slots\":[";for(size_t i=0;i<key.size();++i)row+=(i?",":"")+std::to_string(key[i]);row+="]}\n";Log(row.c_str());}}
        const std::lock_guard hold(programs.shader_texture_lock);auto& cache=programs.shader_texture_cache;
        if(cache.size()>=8)cache.erase(std::min_element(cache.begin(),cache.end(),[](const auto& a,const auto& b){return a.used<b.used;}));
        entry.used=++programs.shader_texture_clock;cache.push_back(std::move(entry));
    }
    for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage)
        if(const auto& shader=programs.interfaces[recipe][stage])CheckNativeShaderTextureRecords(bound,*shader,bound.shader_textures[recipe][stage]);
}
// C-5 (K-COLLECT-IMPLEMENTATION-SHEET 1.2): the per-frame input signature of an admitted model of the main view
// (placement, instance records, pose and stream versions, cloth frame, texture scroll rows), measurement runs with
// light-observe.flag only: hashing every admitted model's inputs cost 7.3 ms a collect at m45 (1,667 models, Native1834), so a
// plain measurement run (the row's "reuse" zeros) gives the frame rate of normal play (Native1835: 50 frames/s against 34).
void NoteNativeModelReuse(const NativeSelectedModel& model,uint64_t selection_serial) {
    static const bool tally=ModuleFlag(L"light-observe.flag");
    if(!tally || !kprofile::enabled.load(std::memory_order_relaxed) || native_offscreen_preparing)return;
    uint64_t h=1469598103934665603ull;
    const auto mix=[&](const void* data,size_t bytes){const auto* p=static_cast<const uint8_t*>(data);for(size_t i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}};
    mix(model.placement.data(),sizeof(model.placement));
    for(uint32_t fade=0;fade<model.geometry_fades.count && fade<2;++fade){
        mix(model.instances[fade].bytes.data(),model.instances[fade].bytes.size());mix(model.instances[fade].written.data(),model.instances[fade].written.size());}
    const void* versions[3]{model.pose.get(),model.previous_pose.get(),model.streams.get()};mix(versions,sizeof(versions));
    if(model.cloth_world)mix(model.cloth_world->data(),sizeof(*model.cloth_world));
    if(model.texture_scroll)mix(model.texture_scroll->data(),model.texture_scroll->size());
    // C7-M: the mesh set's admission outputs (not the scene, view and instance blocks) and the instance records, apart.
    const auto signature=h;
    h=1469598103934665603ull;
    const auto pointer=[&](const void* p){mix(&p,sizeof(p));};
    for(const auto& mesh:model.meshes){
        mix(&mesh.current,sizeof(mesh.current));pointer(mesh.material.get());pointer(mesh.layout.get());pointer(mesh.programs.get());
        mix(&mesh.geometry.storage_generation,8);mix(&mesh.geometry.stream_generation,8);pointer(mesh.geometry.vertex_resource.Get());
        pointer(mesh.geometry.stream_owner.get());
        for(const auto& write:mesh.numeric_writes){mix(&write.byte_offset,4);mix(&write.bytes,4);mix(write.words.data(),sizeof(write.words));}
        mix(mesh.dynamic_bytes.data(),mesh.dynamic_bytes.size());mix(mesh.dynamic_written.data(),mesh.dynamic_written.size());
        const int wet=mesh.wet_activation?int(*mesh.wet_activation):-1;mix(&wet,sizeof(wet));
        for(const auto& binding:mesh.texture_bindings)pointer(binding.texture.get());
        for(const auto& binding:mesh.scene_texture_bindings){mix(&binding.slot,4);pointer(binding.texture.get());}
        for(const auto& recipe:mesh.shader_textures)for(const auto& stage:recipe){mix(&stage.status,sizeof(stage.status));
            for(const auto& entry:stage.registers){mix(&entry.slot,4);mix(&entry.binding,sizeof(entry.binding));mix(&entry.source,sizeof(entry.source));}}
        for(const auto& recipe:mesh.numeric_blocks)for(const auto& block:recipe)pointer(block.get());}
    const auto mesh_set=h;
    h=1469598103934665603ull;
    for(uint32_t fade=0;fade<model.geometry_fades.count && fade<2;++fade){
        mix(model.instances[fade].bytes.data(),model.instances[fade].bytes.size());mix(model.instances[fade].written.data(),model.instances[fade].written.size());}
    const auto instances=h;
    struct Seen {uint64_t signature{},mesh_set{},instances{},serial{};};
    static std::mutex lock;static std::unordered_map<uintptr_t,Seen> seen;static uint64_t last_sweep{};
    const std::lock_guard hold(lock);
    const auto [entry,fresh]=seen.try_emplace(model.entity,Seen{signature,mesh_set,instances,selection_serial});
    if(fresh)++kprofile::reuse[kprofile::ModelsNew];
    else{++kprofile::reuse[entry->second.signature==signature?kprofile::ModelsUnchanged:kprofile::ModelsChanged];
        ++kprofile::reuse[entry->second.mesh_set==mesh_set?kprofile::MeshSetsUnchanged:kprofile::MeshSetsChanged];
        if(entry->second.instances!=instances)++kprofile::reuse[kprofile::InstancesChanged];
        entry->second={signature,mesh_set,instances,selection_serial};}
    if(selection_serial>=last_sweep+120){last_sweep=selection_serial;std::erase_if(seen,[&](const auto& item){return item.second.serial+120<selection_serial;});}
}
// C7-2 (PERSISTENT-RECORDS-DESIGN 1, the per-model collect record): a main-view model whose mesh admission inputs are
// unchanged reuses the mesh set it was given last time, skipping MeshLookups, MeshPrograms, MeshTextures, MeshShaderTex,
// MeshMatDyn and the numeric half of MeshBlocks. The record keeps what that set was admitted from: the model's fields the
// mesh loop reads (storage, LOD selection, channel, decal choice and patch, wet draw inputs, parameter animation rows,
// the current mesh inputs), the global epochs (NativeRecordEpochs), each admitted mesh's material bytes [0, 0x4F8) (every
// word the loop reads at the material, its dynamic object +D0 included), the (holder, texture) pair of each kept texture
// choice (re-read as P4b) and the earliest end of a kept later-candidate or absent choice (P4c). A model is not recorded
// (volatile, recomputed every collect) when a mesh output depends on what these do not cover: a dynamic or cloth stream,
// a FaceGen slot, a texture choice not kept under this collect's epochs, a material provider queried by a virtual call
// (+360 with +20, +370 with +21) or shader-generator arrays reached through pointers (+1A). Guard: every 32nd collect per
// record the model is admitted afresh and compared
// mesh by mesh with its record (native_model_record_mismatch); no-model-records.flag turns records off (A/B).
// I-0070 (scene data persistence): the global epochs that move at every arrival while the world streams (model storage,
// materials and layouts; shaders, effects and materials; the texture registry and ingress) no longer invalidate every record:
// a record checks, when one of them moved since its last check, the registry entries its meshes were resolved from, their
// programmes, and its later-candidate and absent texture choices (looked up again); only what changed for this model makes
// it admitted afresh. Native2325 (the Firelink walk): one arrival re-admitted every visible model and dropped the others.
constexpr size_t native_material_record_bytes=0x4F8;
// Optimisation review (Thomas, 2026-10-19: optimise, do not stack): a reused record is admitted afresh and compared once every
// this many collects (spread by entity). At 32 it was 52 of the 61 fresh admissions a collect at m45 (Native2276) and found no
// mismatch in the 80 native runs before it; 512 keeps the check (every 8.5 s per model at 60 frames/s). I-0070's proof runs
// take 8 with record-revalidation.flag (the targeted checks against a fresh admission while the world streams).
// I-0078 (i74-native-75: the record check was a fifth of a reused model's admission, 2,936 meshes comparing their materials'
// 0x4F8 bytes each collect at m45, many of them the same materials): a material's record bytes, kept with it and compared with
// the engine's at most once per main collect for all the records using it; a change makes a new copy, so a record admitted from
// an earlier copy sees its material changed. Null when unreadable. Called under model_material_lock (shared).
std::shared_ptr<const std::vector<uint8_t>> NativeMaterialRecordBytes(const NativeModelMaterial& material,uintptr_t address,uint64_t collect_serial) {
    auto& kept=material.record_bytes;const std::lock_guard hold(kept.lock);
    if(kept.bytes && kept.address==address && kept.serial==collect_serial)return kept.bytes;
    if(!kept.bytes || kept.address!=address || !GuardedSame(address,kept.bytes->data(),native_material_record_bytes)){
        auto bytes=std::make_shared<std::vector<uint8_t>>(native_material_record_bytes);
        if(!Read(address,bytes->data(),bytes->size())){kept.bytes.reset();return nullptr;}
        kept.bytes=std::move(bytes);kept.address=address;}
    kept.serial=collect_serial;return kept.bytes;
}
uint64_t NativeRecordRevalidationPeriod() noexcept {static const uint64_t period=ModuleFlag(L"record-revalidation.flag")?8:512;return period;}
enum NativeRecordOutcome : uint8_t {RecordReused,RecordBuilt,RecordRevalidated,RecordMismatch,RecordEpochs,RecordModel,RecordInputs,
    RecordParameters,RecordMaterial,RecordHolder,RecordRegistry,RecordProgrammes,RecordWithdrawn,RecordStreams,RecordFaceGen,
    RecordProvider,RecordGenerator,RecordPrograms,RecordRebound,RecordRechecked,RecordOutcomeCount};
// holder: a binding's holder is no longer a SprjGxTexture, or a lightmap set word moved; registry: a mesh's material or layout
// entry changed; programmes: a mesh's programme choice changed; withdrawn: a bound texture record was withdrawn; rebound
// (I-0071): a holder's inner changed and the binding was rebound in place; rechecked: the registry entries, programmes and
// withdrawals rechecked and unchanged (rebound and rechecked are then reused or checked on).
constexpr const char* native_record_outcome_names[RecordOutcomeCount]{"reused","built","revalidated","mismatch","epochs","model","inputs",
    "parameters","material","holder","registry","programmes","withdrawn","streams","facegen","provider","generator",
    "programs","rebound","rechecked"};
struct NativeModelRecord {
    uintptr_t model{};
    std::shared_ptr<const NativeModelStorage> geometry;uintptr_t storage{};   // storage: its model_storage key (I-0070, the unseen sweep)
    uint32_t lods{};bool prefer_channel{},neutral_instance_decal{};
    std::optional<ingress::ModelDecalChoice> decal_choice;uintptr_t patch{},patch_table{};
    std::optional<ingress::ModelWetDrawInputs> wet_draw_inputs;
    ds3content::ModelParameterAnimations parameter_animations;
    std::vector<ingress::CurrentModelMesh> inputs;
    NativeRecordEpochs epochs;
    // native_material_record_bytes per mesh of the set, in set order: I-0078, the material's kept copy (NativeMaterialRecordBytes),
    // so a reused record checks its meshes' materials by identity and each material's bytes are compared once per collect.
    std::vector<std::shared_ptr<const std::vector<uint8_t>>> materials;
    std::shared_ptr<const NativeSelectedModel::EngineBytes> engine_bytes;   // I-0078: the engine bytes of its last admission
    bool material_union_known{};std::optional<std::array<uint32_t,2>> material_union;   // I-0078: its meshes' classes and flags
    std::vector<std::pair<uintptr_t,uintptr_t>> lightmap_words;   // lightmap floors: the words the draw's lightmap set was read from
    bool water{};   // I-0070: a mesh binds the water slots (t54 the reflection: compared with epochs.reflection)
    bool snow{};    // I-0105 tranche 10: a mesh's programme reads t47..t51 (compared with epochs.snow)
    std::shared_ptr<const bootstrap::NativeTextureRecord> paint;   // I-0120 B1: its receiver's paint view (t41) read before its admission
    NativeMeshSet meshes;   // its bindings carry their holders (I-0071): what they hold is checked at each reuse
    uint64_t serial{};
};
struct NativeModelRecords {
    std::mutex lock;   // last, catalog, collects (the entries have their own shard locks)
    // By entity (I-0077: in shards; a record taken out leaves its slot empty, put back in place). Written by the admission
    // workers, swept by the main collect's end, cleared by Reset from other threads.
    NativeEntityShards<std::optional<NativeModelRecord>> entries;
    std::optional<NativeRecordEpochs> last;uint64_t last_sweep{};
    std::shared_ptr<const std::vector<std::wstring>> catalog;uint64_t catalog_epoch{};   // B8: one object for the session once read
    std::array<std::atomic<uint64_t>,RecordOutcomeCount> outcomes{};uint64_t collects{};   // outcomes: counted by the admission workers (I-0071)
    std::atomic<uint64_t> resets{};   // review: a record taken out before a Reset is not put back after it (incremented before the shards are cleared)
};
NativeModelRecords native_model_records;
bool native_model_records_disabled{};
void ResetNativeModelRecords() {
    ++native_model_records.resets;   // first: a record put back after this is refused, one put back before is cleared below
    native_model_records.entries.Clear();
    ++native_model_records.resets;   // review L3: and one taken after the first increment from a shard not yet cleared
    const std::lock_guard hold(native_model_records.lock);native_model_records.last.reset();
}
void NoteNativeRecordOutcome(NativeRecordOutcome outcome) noexcept {
    native_model_records.outcomes[outcome].fetch_add(1,std::memory_order_relaxed);
    static constexpr kprofile::Counter counters[RecordOutcomeCount]{kprofile::ModelRecordsReused,kprofile::ModelRecordsBuilt,
        kprofile::ModelRecordsRevalidated,kprofile::ModelRecordMismatches};
    if(outcome==RecordRebound || outcome==RecordRechecked)return;
    if(outcome<=RecordMismatch)kprofile::Add(counters[outcome]);
    else if(outcome<RecordStreams)kprofile::Add(kprofile::ModelRecordsInvalid);else kprofile::Add(kprofile::ModelRecordsVolatile);
}
// The main collect's epochs, read before the walk (a change during the collect makes this collect's records stale).
void CaptureNativeRecordEpochs(NativeSelectedScene& scene) {
    if(native_model_records_disabled)return;
    NativeRecordEpochs epochs;epochs.serial=scene.uid;   // review M1: unique per collect (the frame serial repeats when a collect runs twice before Clear())
    epochs.models=model_registry_epoch.load(std::memory_order_acquire);
    epochs.programs=program_registry_generation.load(std::memory_order_acquire);
    epochs.texture_withdrawals=textures.Withdrawals();
    epochs.reflection=NativeReflectionRecord();   // I-0070: t54 changes once (the reflection's first picture); before, any epoch churn refreshed it
    epochs.snow=scene.snow_textures;   // I-0105 tranche 10: collected before (CollectNativeSnowTextures)
    if(!Read(image+0x4792F58,&epochs.named_registry,8) || !Read(image+0x4929940,epochs.constants.data(),16) ||
       !Read(image+0x3D7FEF0,epochs.constants.data()+16,16))return;
    {uintptr_t settings{};   // C2: as the water block reads it (1 while unreadable)
        if(Read(image+0x4961B50,&settings,8) && settings && Read(settings+0x5360,&settings,8) && settings)Read(settings+0x6C,&epochs.reflection_option,1);}
    epochs.common_images=scene.common_images;
    {const std::lock_guard hold(native_scene_copy_lock);epochs.scene_copy=native_scene_copy;}
    {const std::lock_guard hold(native_light_grid_lock);epochs.light_grid=native_light_grid;}   // I-0107
    // I-0070 review: the current t56 record (made again after a resize), not the last one a fresh water admission made: reused
    // water records no longer churn, so the cached one would stay at the old size. None before any water was admitted.
    {std::shared_ptr<const bootstrap::NativeTextureRecord> depth;{const std::lock_guard hold(native_water_lock);depth=native_water_depth;}
        epochs.water_depth=depth?NativeWaterDepthRecord():nullptr;}
    {const std::lock_guard hold(native_model_records.lock);auto& records=native_model_records;
        if(records.catalog!=scene.wet_shader_catalog){records.catalog=scene.wet_shader_catalog;++records.catalog_epoch;}
        epochs.catalog=records.catalog_epoch;}
    scene.record_epochs=std::move(epochs);
}
bool EqualParameterAnimations(const ds3content::ModelParameterAnimations& a,const ds3content::ModelParameterAnimations& b) {
    if(a.available!=b.available || a.rows.size()!=b.rows.size())return false;
    for(size_t i=0;i<a.rows.size();++i){const auto& x=a.rows[i];const auto& y=b.rows[i];
        if(x.lookup_key!=y.lookup_key || x.universal!=y.universal || x.target!=y.target || x.strength!=y.strength || x.kind!=y.kind ||
           x.material_index!=y.material_index || x.material_name!=y.material_name)return false;}
    return true;
}
bool EqualCurrentMeshes(std::span<const ingress::CurrentModelMesh> a,std::span<const ingress::CurrentModelMesh> b) {
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i){const auto& x=a[i];const auto& y=b[i];
        if(x.ordinal!=y.ordinal || x.flags!=y.flags || x.layout_type!=y.layout_type || x.material_id!=y.material_id ||
           x.patch_index!=y.patch_index || x.lod_mask!=y.lod_mask || x.material!=y.material || x.layout!=y.layout)return false;}
    return true;
}
// I-0070: the material and layout entries a record's meshes were resolved from are still the registries' (an insert or erase
// elsewhere moved model_registry_epoch; the storage is compared by the admission itself).
bool SameNativeRecordRegistries(const NativeModelRecord& record) {
    {const std::shared_lock hold(model_material_lock);
        for(const auto& mesh:record.meshes){const auto found=model_materials.find(mesh.current.material);
            if(found==model_materials.end() || found->second!=mesh.material)return false;}}
    const std::shared_lock hold(model_layout_lock);
    for(const auto& mesh:record.meshes){const auto found=model_layouts.find(mesh.current.layout);
        if(found==model_layouts.end() || found->second!=mesh.layout)return false;}
    return true;
}
// I-0070: each mesh's programme choice is the one the admission would make now (a shader, effect or material change elsewhere
// moved program_registry_generation; the layout's kept choice makes this a lookup once per layout and generation).
bool SameNativeRecordPrograms(const NativeModelRecord& record) {
    for(const auto& mesh:record.meshes){std::shared_ptr<NativeModelLayout> layout;
        {const std::shared_lock hold(model_layout_lock);const auto found=model_layouts.find(mesh.current.layout);
            if(found==model_layouts.end() || found->second!=mesh.layout)return false;layout=found->second;}
        if(AcquireNativeModelPrograms(mesh.material,layout,record.prefer_channel)!=mesh.programs)return false;}
    return true;
}
// I-0070: a texture record one of the record's meshes binds (material, lightmap or scene slot) was withdrawn by the ingress
// (its receiver retired or admitted again); checked only when TextureIngress::Withdrawals moved since the record's check.
bool NativeRecordTextureWithdrawn(const NativeModelRecord& record) noexcept {
    const auto gone=[](const std::shared_ptr<const bootstrap::NativeTextureRecord>& texture){return texture && texture->withdrawn.load(std::memory_order_acquire);};
    for(const auto& mesh:record.meshes){
        for(const auto& binding:mesh.texture_bindings)if(gone(binding.texture))return true;
        for(const auto& binding:mesh.scene_texture_bindings)if(gone(binding.texture))return true;}
    return false;
}
// I-0071 (ER B, D-0195): the record's bindings against what their holders hold now. A binding whose holders' inners all
// stand keeps its texture; a changed inner (a texture arrived, or went back to SYSTEX_DUMMY) rebinds that binding in a copy
// of the mesh set, and that mesh's shader texture selection and checks run again: the model is not admitted afresh (as ER's
// next bind reads CSGxTexture's new inner). A holder no longer a SprjGxTexture makes the model admitted afresh (its holders
// are then looked up again). `scene` null (the reflection's borrowed copy): a change refuses the copy instead.
// The common case in one guarded block (i71b m45: a guarded read per word cost the walks 0.7 ms a collect): true when every
// binding's holders still hold the inners recorded, as NativeHolderInner reads them; false on any change or unreadable word,
// and the binding loop below then decides.
__declspec(noinline) static bool NativeMeshHoldersStand(const NativeSelectedMesh::TextureBinding* bindings,size_t count,uintptr_t table) noexcept {
    const bool outer=guarded_copy_active;guarded_copy_active=true;bool stand=true;
    __try {
        for(size_t j=0;j<count && stand;++j){
            if(bindings[j].looked_up && !bindings[j].holders[bindings[j].set_first?1:0]){stand=false;break;}   // its first candidate is looked up again below
            for(size_t k=0;k<bindings[j].holders.size() && stand;++k){uintptr_t at=bindings[j].holders[k];if(!at)continue;
            for(int depth=0;depth<4 && at;++depth){
                if(at<0x10000){stand=false;break;}
                if(*reinterpret_cast<const uintptr_t*>(at)!=table){if(!depth)stand=false;break;}
                at=*reinterpret_cast<const uintptr_t*>(at+0x10);}
            if(stand && at!=bindings[j].inners[k])stand=false;}}
    } __except(EXCEPTION_EXECUTE_HANDLER) {stand=false;}
    guarded_copy_active=outer;return stand;
}
NativeRecordOutcome RebindNativeRecordTextures(NativeModelRecord& record,NativeSelectedScene* scene) {
    std::shared_ptr<std::vector<NativeSelectedMesh>> copy;
    for(size_t i=0;i<record.meshes.size();++i){
        const auto& mesh=record.meshes[i];bool dirty=false;
        if(NativeMeshHoldersStand(mesh.texture_bindings.data(),mesh.texture_bindings.size(),image+0x29559E8))continue;
        for(size_t j=0;j<mesh.texture_bindings.size();++j){const auto& binding=mesh.texture_bindings[j];
            const size_t own=binding.set_first?1:0;   // the lookup candidates' first holder
            // Review M3: a first candidate without a holder (no cap yet) is looked up again as the admission does (the kept
            // choice, renewed after 500 ms or a registry move); new holders rebind the binding.
            std::array<uintptr_t,4> holders=binding.holders;
            if(binding.looked_up && !holders[own] && mesh.programs && mesh.layout && j<mesh.programs->texture_lookups.size()){uintptr_t registry{};
                bool busy{};   // C1: a lookup given up leaves the binding as it is until the next check
                if(Read(image+0x4792F58,&registry,8) && registry){const auto fresh=NativeTextureHolders(*mesh.programs,j,mesh.layout->resolver,registry,&busy);if(busy)continue;
                    for(size_t k=own;k<holders.size();++k)holders[k]=fresh[k-own];}}
            if(std::all_of(holders.begin(),holders.end(),[](uintptr_t h){return h==0;}))continue;
            std::array<uintptr_t,4> now{};
            for(size_t k=0;k<now.size();++k)if(holders[k]){bool valid{};now[k]=NativeHolderInner(holders[k],&valid);
                if(!valid){   // review M3: the kept choice is dropped, so the fresh admission looks the holders up again
                    if(mesh.programs && j<mesh.programs->texture_choices.size()){const std::lock_guard hold(mesh.programs->texture_choice_lock);mesh.programs->texture_choices[j]={};}
                    return RecordHolder;}}
            if(holders==binding.holders && now==binding.inners)continue;
            if(!scene || !mesh.programs || !mesh.layout || j>=mesh.programs->texture_lookups.size())return RecordHolder;
            if(!copy)copy=std::make_shared<std::vector<NativeSelectedMesh>>(*record.meshes.items);
            auto& rebound=(*copy)[i];auto& target=rebound.texture_bindings[j];size_t chosen=~size_t{};target.holders=holders;
            auto texture=ResolveNativeTextureHolders(target.holders,target.inners,&chosen);
            if(target.dol_constant){rebound.lightmap_constant.reset();   // the DOL t26 lightmap's constant, as ApplyNativeLightmapSet
                if(texture && chosen==0){const std::lock_guard hold(native_texture_constant_lock);
                    if(const auto found=native_texture_constants.find(target.inners[0]);found!=native_texture_constants.end())rebound.lightmap_constant=found->second;}}
            if(!texture){EnsureNativeSystemTextures(*scene);
                texture=NativeFallbackTexture(target.input,mesh.programs->texture_lookups[j],scene->system_textures,scene->common_images!=nullptr,
                    rebound,record.model,mesh.layout->resolver,target.holders[own],target.inners[own]);}
            target.texture=std::move(texture);dirty=true;}
        if(dirty)SelectNativeMeshShaderTextures((*copy)[i],nullptr);
    }
    if(copy){record.meshes.items=std::move(copy);NoteNativeRecordOutcome(RecordRebound);}
    return RecordReused;
}
// The model-level part of the check: the record's inputs against this collect's (the material bytes and bindings follow).
NativeRecordOutcome CheckNativeModelRecord(NativeModelRecord& record,const NativeSelectedModel& selected,const NativeSelectionCandidate& c,
    const ingress::CurrentModelInputs& inputs,uint32_t lods,const ds3content::ModelParameterAnimations& animations,const NativeRecordEpochs& epochs,
    NativeSelectedScene* scene) {
    // I-0120 B1: a paint target made or retired re-admits only the record of its own receiver (the bloody provider's t41/t42,
    // 3D41DF0), not every record of the scene (a hitch at each first hit on an enemy).
    const bool painted=record.decal_choice==ingress::ModelDecalChoice::InstanceProvider && record.patch && record.patch_table==image+0x3D41DF0;
    if(!record.epochs.SameStable(epochs) || (record.water && record.epochs.reflection!=epochs.reflection) ||
       (record.snow && record.epochs.snow!=epochs.snow) || (painted && NativePaintViewsOf(record.patch).diffuse!=record.paint))return RecordEpochs;
    if(selected.streams || c.cloth_set)return RecordStreams;
    uintptr_t patch_table{};if(selected.patch && !Read(selected.patch,&patch_table,8))return RecordModel;
    if(record.model!=selected.model || record.geometry!=selected.geometry || record.lods!=lods ||
       record.prefer_channel!=!(selected.fade.group_flags&2u) || record.neutral_instance_decal!=selected.neutral_instance_decal ||
       record.decal_choice!=selected.decal_choice || record.patch!=selected.patch || record.patch_table!=patch_table ||
       record.wet_draw_inputs.has_value()!=selected.wet_draw_inputs.has_value() ||
       (record.wet_draw_inputs && std::memcmp(&*record.wet_draw_inputs,&*selected.wet_draw_inputs,sizeof(ingress::ModelWetDrawInputs))))return RecordModel;
    if(!EqualCurrentMeshes(record.inputs,inputs.meshes))return RecordInputs;
    if(!EqualParameterAnimations(record.parameter_animations,animations))return RecordParameters;
    // I-0070: the tracked epochs, each value read before its check (a change during the check is seen by the next collect).
    if(const auto models=model_registry_epoch.load(std::memory_order_acquire);models!=record.epochs.models){
        if(!SameNativeRecordRegistries(record))return RecordRegistry;
        record.epochs.models=models;NoteNativeRecordOutcome(RecordRechecked);}
    if(const auto programs=program_registry_generation.load(std::memory_order_acquire);programs!=record.epochs.programs){
        if(!SameNativeRecordPrograms(record))return RecordProgrammes;
        record.epochs.programs=programs;NoteNativeRecordOutcome(RecordRechecked);}
    if(const auto withdrawals=textures.Withdrawals();withdrawals!=record.epochs.texture_withdrawals){
        if(NativeRecordTextureWithdrawn(record))return RecordWithdrawn;
        record.epochs.texture_withdrawals=withdrawals;NoteNativeRecordOutcome(RecordRechecked);}
    {const std::shared_lock hold(model_material_lock);   // as the admission's reads: the registry gates the prefix's retirement
        if(record.materials.size()!=record.meshes.size())return RecordMaterial;
        for(size_t i=0;i<record.meshes.size();++i){const auto& mesh=record.meshes[i];
            if(!mesh.material || !record.materials[i] || NativeMaterialRecordBytes(*mesh.material,mesh.current.material,epochs.serial)!=record.materials[i])return RecordMaterial;}}
    for(const auto& [address,value]:record.lightmap_words){uintptr_t current{};if(!Read(address,&current,8) || current!=value)return RecordHolder;}
    return RebindNativeRecordTextures(record,scene);
}
// What makes one freshly admitted mesh unrecordable (RecordReused when nothing), from its material bytes and its programme.
NativeRecordOutcome NoteNativeRecordMesh(const NativeSelectedMesh& bound,std::span<const uint8_t> material,NativeModelRecord& record) {
    if(bound.texture_gave_up)return RecordHolder;   // I-0120 C1
    const auto* source=material.data()+0xD0;uintptr_t colour_provider{},id_provider{};
    std::memcpy(&colour_provider,source+0x360,8);std::memcpy(&id_provider,source+0x370,8);
    if((source[0x20] && colour_provider) || (source[0x21] && id_provider))return RecordProvider;
    if(source[0x1A])return RecordGenerator;
    if(!bound.programs)return RecordPrograms;   // review: a failed programme choice is never kept (K P6)
    if(bound.programs->policy && (bound.programs->policy->flags&0x30))record.water=true;
    if(bound.programs->samples_snow)record.snow=true;
    if(!bound.programs->texture_bindings_available)return RecordReused;
    for(const auto& lookup:bound.programs->texture_lookups)if(lookup.facegen)return RecordFaceGen;
    return RecordReused;
}
bool EqualRegisterBlocks(const SharedRegisterBlock& a,const SharedRegisterBlock& b) {
    if(a==b)return true;if(!a || !b)return false;
    return a->status==b->status && a->required==b->required && a->bytes==b->bytes;
}
// The revalidation's comparison: the first field in which two admissions of one mesh differ, null when none.
const char* NativeMeshDifference(const NativeSelectedMesh& a,const NativeSelectedMesh& b) {
    if(!EqualCurrentMeshes({&a.current,1},{&b.current,1}))return "current";
    if(std::memcmp(&a.slice,&b.slice,sizeof(a.slice)))return "slice";
    if(a.material!=b.material)return "material";if(a.layout!=b.layout)return "layout";if(a.programs!=b.programs)return "programs";
    if(a.lightmap_constant!=b.lightmap_constant)return "lightmap_constant";
    const auto& g=a.geometry;const auto& h=b.geometry;
    if(g.storage_generation!=h.storage_generation || g.stream_generation!=h.stream_generation || g.vertex_resource.Get()!=h.vertex_resource.Get() ||
       g.index_resource.Get()!=h.index_resource.Get() || std::memcmp(&g.vertices,&h.vertices,sizeof(g.vertices)) || std::memcmp(&g.vertices2,&h.vertices2,sizeof(g.vertices2)) ||
       std::memcmp(&g.indices,&h.indices,sizeof(g.indices)) || g.index_count!=h.index_count || g.stream_owner!=h.stream_owner ||
       a.cloth_previous.Get()!=b.cloth_previous.Get())return "geometry";   // I-0134: the Velo rows' previous copy
    if(!EqualNumericWrites(a.numeric_writes,b.numeric_writes))return "numeric_writes";
    if(a.dynamic_bytes!=b.dynamic_bytes || a.dynamic_written!=b.dynamic_written)return "dynamic";
    if(a.parameter_animations_available!=b.parameter_animations_available || a.parameter_animation_rows!=b.parameter_animation_rows)return "parameter_animations";
    if(a.wet_activation!=b.wet_activation || a.material_class!=b.material_class || a.current_special!=b.current_special || a.no_cast!=b.no_cast ||
       a.base_deformation!=b.base_deformation || a.material_index!=b.material_index || a.cloth_stream!=b.cloth_stream)return "material_fields";
    if(a.material_masks.has_value()!=b.material_masks.has_value() || (a.material_masks && (a.material_masks->first!=b.material_masks->first ||
       a.material_masks->second!=b.material_masks->second || a.material_masks->secondary!=b.material_masks->secondary)))return "material_masks";
    if(a.pose_policy.has_value()!=b.pose_policy.has_value() || (a.pose_policy && (a.pose_policy->cloth_enabled!=b.pose_policy->cloth_enabled ||
       a.pose_policy->flags!=b.pose_policy->flags)))return "pose_policy";
    if(a.wet_parameters.has_value()!=b.wet_parameters.has_value() || (a.wet_parameters && (a.wet_parameters->diffuse!=b.wet_parameters->diffuse ||
       a.wet_parameters->reflectance!=b.wet_parameters->reflectance || a.wet_parameters->type!=b.wet_parameters->type ||
       a.wet_parameters->scale!=b.wet_parameters->scale || a.wet_parameters->override_abc!=b.wet_parameters->override_abc)))return "wet_parameters";
    if(a.texture_bindings.size()!=b.texture_bindings.size())return "texture_bindings";
    for(size_t i=0;i<a.texture_bindings.size();++i)if(a.texture_bindings[i].texture!=b.texture_bindings[i].texture ||
        a.texture_bindings[i].input.declaration.slot!=b.texture_bindings[i].input.declaration.slot || a.texture_bindings[i].input.path!=b.texture_bindings[i].input.path)return "texture_binding";
    if(a.scene_texture_bindings.size()!=b.scene_texture_bindings.size())return "scene_bindings";
    for(size_t i=0;i<a.scene_texture_bindings.size();++i)if(a.scene_texture_bindings[i].slot!=b.scene_texture_bindings[i].slot ||
        a.scene_texture_bindings[i].texture!=b.scene_texture_bindings[i].texture)return "scene_binding";
    for(size_t recipe=0;recipe<native_recipe_count;++recipe)for(size_t stage=0;stage<2;++stage){
        const auto& x=a.shader_textures[recipe][stage];const auto& y=b.shader_textures[recipe][stage];
        if(x.status!=y.status || x.registers.size()!=y.registers.size())return "shader_textures";
        for(size_t i=0;i<x.registers.size();++i)if(x.registers[i].slot!=y.registers[i].slot || x.registers[i].binding!=y.registers[i].binding ||
            x.registers[i].source!=y.registers[i].source)return "shader_textures";
        if(!EqualRegisterBlocks(a.numeric_blocks[recipe][stage],b.numeric_blocks[recipe][stage]))return "numeric_blocks";
    }
    return nullptr;
}
// End of a main collect: when the stable epochs moved, the
// records this collect did not visit are stale and go; a witness row every 600. I-0070 (camera turns, Native2378 / the
// Archdragon Peak sweeps): every 120 collects, a record unseen for 120 or more is kept while what it holds is still current
// (its storage is the registry's, none of its textures withdrawn), up to native_record_keep_unseen collects and
// native_record_keep_cap records (the oldest unseen go first), so a model coming back into view is checked, not admitted
// afresh; a retired storage or texture still leaves within 120 collects, as before (its GPU lifetime is unchanged).
constexpr uint64_t native_record_keep_unseen=3600;constexpr size_t native_record_keep_cap=8192;
void FinishNativeModelRecords(const NativeSelectedScene& scene) {
    if(native_model_records_disabled)return;
    struct Unseen {uintptr_t entity{},storage{};const void* geometry{};};std::vector<Unseen> unseen;
    {const std::lock_guard hold(native_model_records.lock);auto& records=native_model_records;
        // An empty slot (its record taken by an admission that then ended without one) goes with the first sweep that sees it.
        if(!scene.record_epochs)records.entries.Clear();
        else if(!records.last || !records.last->SameStable(*scene.record_epochs))   // the tracked epochs are checked by each record
            records.entries.EraseIf([&](const auto& item){return !item.second || item.second->serial!=scene.serial;});
        if(scene.serial>=records.last_sweep+120){records.last_sweep=scene.serial;
            records.entries.EraseIf([&](const auto& item){if(!item.second)return true;const auto age=scene.serial-item.second->serial;
                return age>=native_record_keep_unseen || (age>=120 && NativeRecordTextureWithdrawn(*item.second));});
            std::vector<std::pair<uint64_t,uintptr_t>> oldest;size_t total{};
            for(auto& shard:records.entries.shards){const std::lock_guard held(shard.lock);total+=shard.map.size();
                for(const auto& [entity,record]:shard.map)if(record && scene.serial-record->serial>=120){
                    unseen.push_back({entity,record->storage,record->geometry.get()});oldest.emplace_back(record->serial,entity);}}
            if(total>native_record_keep_cap){
                const auto excess=(std::min)(total-native_record_keep_cap,oldest.size());
                std::partial_sort(oldest.begin(),oldest.begin()+excess,oldest.end());
                for(size_t i=0;i<excess;++i){auto& shard=records.entries.Of(oldest[i].second);const std::lock_guard held(shard.lock);shard.map.erase(oldest[i].second);}}}
        records.last=scene.record_epochs;
        if(++records.collects%600==0){std::string row="{\"event\":\"native_model_records\",\"collects\":"+std::to_string(records.collects)+
            ",\"records\":"+std::to_string(records.entries.Size())+",\"outcomes\":{";
            for(size_t i=0;i<RecordOutcomeCount;++i)row+=(i?",\"":"\"")+std::string(native_record_outcome_names[i])+"\":"+std::to_string(records.outcomes[i]);
            row+="}}\n";Log(row.c_str());}}
    if(unseen.empty())return;
    // The storage registry is read outside the records' lock (no lock order between them); an entry admitted again since
    // (seen in this collect) is not touched.
    std::vector<uintptr_t> stale;
    {const std::shared_lock hold(model_storage_lock);   // I-0081: a lookup, shared
        for(const auto& item:unseen){const auto found=model_storage.find(item.storage);
            if(found==model_storage.end() || found->second.get()!=item.geometry)stale.push_back(item.entity);}}
    if(stale.empty())return;
    for(const auto entity:stale){auto& shard=native_model_records.entries.Of(entity);const std::lock_guard hold(shard.lock);
        if(const auto found=shard.map.find(entity);found!=shard.map.end() && (!found->second || scene.serial-found->second->serial>=120))shard.map.erase(found);}
}
}  // namespace
