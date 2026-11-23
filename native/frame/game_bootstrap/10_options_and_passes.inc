// Graphics options, the quality object, the display curve, the pass preparation of the selected scene, the shadow state and
// the glow and AO inputs. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every
// earlier part (09_model_admission.inc before it).
#pragma once
#include "09_model_admission.inc"
namespace {
// 1255 (I-0067 QUALITY-OBJECT-NATIVE Q4): CED9A0 -> 23660 -> 23790 natively, from the render step that already reads the
// graphics options: the levels at [4796298]+0xA8 + 4 x slot (slot 8 has no file key: CED9A0 passes the applied copy),
// read twice and taken only when both reads agree (O2: ReadNativeOptionLevels) and every level is 0..4 (the original reads
// the table unchecked: a declared safety deviation); 23660's lock byte +5350 is honoured; the object is rewritten only when
// a level changed. 23790's gate [144961AB0]+0x3E1 is 0 natively (no old device); its sampler call D3650 (Q6) and listener
// loop +4E08 have no native peer.
void UpdateNativeQualityObject(std::array<int32_t,12> levels) {
    uintptr_t root{},table{},object{};BYTE lock{};
    if(!Read(image+0x4961B50,&root,8) || !root || !Read(root+0x5358,&table,8) || !table || !Read(root+0x5360,&object,8) || !object ||
       !Read(root+0x5350,&lock,1) || lock)return;
    std::array<int32_t,12> applied{};
    if(!Read(root+0x52F0,applied.data(),sizeof(applied)))return;
    levels[8]=applied[8];
    if(std::any_of(levels.begin(),levels.end(),[](int32_t level){return level<0 || level>4;})){
        static LONG64 refused{};if(InterlockedIncrement64(&refused)<=4)Log("{\"event\":\"native_quality_object\",\"edge\":\"level_refused\"}\n");return;}
    std::memcpy(reinterpret_cast<void*>(root+0x5320),levels.data(),sizeof(levels));
    if(levels==applied)return;
    bootstrap::WriteQualityObject(reinterpret_cast<const uint8_t*>(table),levels,reinterpret_cast<uint8_t*>(object));
    std::memcpy(reinterpret_cast<void*>(root+0x52F0),levels.data(),sizeof(levels));
    static LONG64 updates{};if(InterlockedIncrement64(&updates)<=16){std::string list;
        for(const auto level:levels)list+=(list.empty()?"":",")+std::to_string(level);
        std::array<uint8_t,bootstrap::QualityObjectBytes> bytes{};Read(object,bytes.data(),bytes.size());std::string hex;
        for(const auto byte:bytes){char cell[4]{};sprintf_s(cell,"%02x",byte);hex+=cell;}
        const std::string row="{\"event\":\"native_quality_object\",\"edge\":\"updated\",\"levels\":["+list+"],\"object\":\""+hex+"\"}\n";Log(row.c_str());}
}
// O2: the one read of the twelve level words (797: D55190 -> DFDD20(owner) -> CED9A0(owner+A0) reads them, the Texture
// selector first): the options owner the same before and after, the words read twice and equal. Repeated reads bound this
// snapshot; CPU writer synchronization and the original editor override remain outside the qualified input family.
bool ReadNativeOptionLevels(uintptr_t settings,std::array<int32_t,12>& levels) noexcept {
    uintptr_t current{},after{};std::array<int32_t,12> again{};
    return settings && Read(image+0x4796298,&current,8) && current==settings && Read(settings+0xA8,levels.data(),sizeof(levels)) &&
        Read(settings+0xA8,again.data(),sizeof(again)) && levels==again && Read(image+0x4796298,&after,8) && after==settings;
}
// The sampler bank of the Texture selector (slot 0 of the levels just read), without the omitted worker's GetDesc
// population or its local Ignore External Settings alias. O1 (options sheet M-5): a read that failed or disagreed keeps
// the last valid bank (no frame without a 3D scene, no new generation for the same selector); an out-of-range selector
// withdraws it.
std::shared_ptr<const ds3content::EngineSamplerBankVersion> NativeSamplerBankOf(bool read,uint32_t selector) {
    if(!read){auto kept=native_sampler_bank.Acquire();
        static LONG64 transient{};if(InterlockedIncrement64(&transient)%600==1){char row[128]{};
            sprintf_s(row,"{\"event\":\"native_sampler_bank_read_failed\",\"kept\":%s}\n",kept?"true":"false");Log(row);}
        return kept;}
    auto version=native_sampler_bank.PublishExternalSelector(selector);
    if(!version)native_sampler_bank.Withdraw();
    return version;
}
// I-0111 tranche 3 (UPSCALER-DLSS-READY §3.8): while DLSS runs, the main view's material samplers take the mode's mip bias,
// log2(render width / output width) - 1 (-1 at DLAA, about -1.585 at Quality): a bank derived from the scene's, its banks 12 (the
// material table's s0..s3, s5 and s10: 10AAD0's 12,12,12,12,14,12,0,3,1,4,12,13,15) and 13 (s11) biased, every other one (point,
// clamp, shadow, the screen-space lookups) as it is. Kept while its source and bias stay, so the draws' sampler tables and the kept
// draws keep their identity; a new one has its own generation. The menu views keep the source bank.
std::shared_ptr<const ds3content::EngineSamplerBankVersion> NativeMaterialBank(const std::shared_ptr<const ds3content::EngineSamplerBankVersion>& source) {
    // I-0132 R11j (the step review's L2): the TAA's frames -0.5 (at the output extent: half DLAA's -1, a hand-written resolve
    // keeping less detail than DLSS's model before it shimmers; ER's own bias not found, ER-TAA-STUDY).
    if(!source || !(NativeDlssActive() || NativeTaaActive()) || !output_width)return source;
    const float bias=NativeDlssActive()?float(std::log2(double(NativeRenderWidth())/double(output_width))-1.0):NativeTaaTest().mip_bias;   // I-0138 tranche 9: taa-mip-bias.txt's, else -0.5
    static std::mutex lock;static std::shared_ptr<const ds3content::EngineSamplerBankVersion> kept_source,kept;static float kept_bias{};static uint64_t made{};
    const std::lock_guard hold(lock);
    if(kept && kept_source==source && kept_bias==bias)return kept;
    auto derived=std::make_shared<ds3content::EngineSamplerBankVersion>(*source);
    derived->generation=(uint64_t{1}<<62)+(++made);   // apart from the owner's generations
    for(const size_t bank:{size_t(12),size_t(13)}){auto& d=derived->descriptors[bank];
        d.MipLODBias=std::clamp(d.MipLODBias+bias,float(D3D12_MIP_LOD_BIAS_MIN),float(D3D12_MIP_LOD_BIAS_MAX));}
    kept_source=source;kept_bias=bias;kept=derived;
    char row[200]{};sprintf_s(row,"{\"event\":\"native_material_mip_bias\",\"bias\":%.4f,\"render_width\":%u,\"output_width\":%u,\"generation\":%llu}\n",bias,NativeRenderWidth(),output_width,
        static_cast<unsigned long long>(derived->generation));Log(row);
    return kept;
}
// O2: publishes a snapshot when it differs from the current one, with one native_option_change row naming the changed slots
// (bit 12: the render size; bit 13: the sampler bank alone; bit 14: the object became valid or invalid) and the classes of
// the change (options sheet section 4). The classes are a record for the families that will act on them (a G change alters
// the frame graph's plan key, an R change needs the ring-wide wait); today the consumers act by themselves on the next
// collect: the sampler bank (P), the velocity rows and their history (G, H), the SSS stage (G) and the point-light fade (P).
void PublishNativeOptions(NativeOptionSnapshot next) {
    std::shared_ptr<const NativeOptionSnapshot> previous;
    {const std::lock_guard hold(native_options_lock);previous=native_options;
        if(previous && previous->object_valid==next.object_valid && previous->levels==next.levels && previous->object==next.object &&
           previous->configured_width==next.configured_width && previous->configured_height==next.configured_height &&
           previous->sampler_bank==next.sampler_bank)return;
        next.generation=++native_options_generation;native_options=std::make_shared<const NativeOptionSnapshot>(next);}
    bootstrap::OptionChangeClass change;
    if(previous){
        const bool levels=previous->object_valid && next.object_valid;
        change=bootstrap::ClassifyOptionChange(levels?previous->levels:next.levels,next.levels,
            previous->configured_width!=next.configured_width || previous->configured_height!=next.configured_height);
        if(previous->sampler_bank!=next.sampler_bank && !(change.changed&1u)){change.changed|=1u<<13;change.classes|=bootstrap::OptionParameter;}
        if(previous->object_valid!=next.object_valid){change.changed|=1u<<14;change.classes|=bootstrap::OptionGate|bootstrap::OptionHistory;}}
    static LONG64 rows{};if(InterlockedIncrement64(&rows)>64)return;
    std::string list;for(const auto level:next.levels)list+=(list.empty()?"":",")+std::to_string(level);
    char classes[5]{};bootstrap::OptionClassLetters(change.classes,classes);char row[512]{};
    sprintf_s(row,"{\"event\":\"native_option_change\",\"generation\":%llu,\"first\":%s,\"levels\":[%s],\"object\":%s,\"changed\":%u,\"classes\":\"%s\","
        "\"configured\":[%u,%u],\"sampler_generation\":%llu}\n",static_cast<unsigned long long>(next.generation),previous?"false":"true",list.c_str(),
        next.object_valid?"true":"false",change.changed,classes,next.configured_width,next.configured_height,
        static_cast<unsigned long long>(next.sampler_bank?next.sampler_bank->generation:0));Log(row);
}
// O2: the render step's one read of the graphics options (owner thread): the level words once (ReadNativeOptionLevels), the
// sampler bank from their Texture selector (O1 rules), CED9A0 -> 23660 -> 23790 from the same words, then the snapshot of
// the levels as applied (+52F0, the copy 23790 leaves next to the object it wrote, so the levels and the object always
// agree), the object's bytes and the configured render size S. A failed object read keeps the previous object and levels
// (the O1 rule, applied to the object). Since the object follows the applied copy, a refused level set or the lock byte
// leaves the MotionBlur row where it was (before O2 it followed the live word): closer to the original's 57A20, which
// reads the object.
void UpdateNativeOptions(uintptr_t settings) {
    NativeOptionSnapshot next;if(const auto current=AcquireNativeOptions())next=*current;
    std::array<int32_t,12> live{};const bool read=ReadNativeOptionLevels(settings,live);
    next.sampler_bank=NativeSamplerBankOf(read,static_cast<uint32_t>(live[0]));
    if(read)UpdateNativeQualityObject(live);
    uintptr_t root{},object{};std::array<uint8_t,bootstrap::QualityObjectBytes> bytes{};std::array<int32_t,12> applied{};
    if(Read(image+0x4961B50,&root,8) && root && Read(root+0x5360,&object,8) && object && Read(object,bytes.data(),bytes.size()) &&
       Read(root+0x52F0,applied.data(),sizeof(applied))){next.object=bytes;next.levels=applied;next.object_valid=true;}
    // O1 (options sheet M-3): S (written on WM_SIZE and by the Options page) against the native output, which follows S at
    // the next engine frame since I-0076 (FollowNativeOutputExtent): one row per change of either.
    uint32_t configured[2]{};
    if(Read(image+0x46601E8,configured,sizeof(configured))){next.configured_width=configured[0];next.configured_height=configured[1];
        static uint64_t last{};
        const uint64_t key=(uint64_t(configured[0])<<48)^(uint64_t(configured[1])<<32)^(uint64_t(output_width)<<16)^output_height;
        if(key!=last){const bool first=!last;last=key;const bool match=configured[0]==output_width && configured[1]==output_height;
            if(!first || !match){char row[192]{};
                sprintf_s(row,"{\"event\":\"native_render_size\",\"configured\":[%u,%u],\"output\":[%u,%u],\"match\":%s}\n",configured[0],configured[1],output_width,output_height,match?"true":"false");Log(row);}}}
    PublishNativeOptions(std::move(next));
}
// W3 G0-2 (GAMMA-COLOUR-PATH-READY 3.2): the two DXBC blobs FUN_141933200 creates the gamma pass from, read from the loaded
// image: the size dwords, the DXBC header sizes and the SHA-256 fixed in display_blobs.h must all agree. A mismatch never
// refuses the game: it is logged and the stage stays closed (the frame draws straight into the back buffer).
bool LoadNativeDisplayBlobs(DisplayBlobs& blobs) {
    const auto load=[](const bootstrap::DisplayBlobPin& pin,std::vector<uint8_t>& out,const char* stage){
        uint32_t size{},total{};std::vector<uint8_t> bytes(pin.bytes);char hash[65]{};
        const bool ok=Read(image+pin.size_rva,&size,4) && size==pin.bytes && Read(image+pin.rva,bytes.data(),bytes.size()) &&
            !std::memcmp(bytes.data(),"DXBC",4) && (std::memcpy(&total,bytes.data()+24,4),total==pin.bytes) &&
            ds3content::Sha256(bytes.data(),bytes.size(),hash) && !std::strcmp(hash,pin.sha256);
        char row[256]{};sprintf_s(row,"{\"event\":\"native_display_blob\",\"stage\":\"%s\",\"bytes\":%u,\"identity\":%s}\n",stage,size,ok?"true":"false");Log(row);
        if(ok)out=std::move(bytes);return ok;};
    return load(bootstrap::display_vertex_blob,blobs.vertex,"vertex") && load(bootstrap::display_pixel_blob,blobs.pixel,"pixel");
}
// W3 (GAMMA-COLOUR-PATH-READY 3.3, 3.4): the six values of the render step (src 2.2, dst 2.2 + 0.5 b; identity until the
// first render step), or, with brightness-override.flag (an integer 0..10 in the file, development), b = (v - 5) / 5. On a
// change the engine's generator FUN_1400C83D0 makes the curve (bit-identical to the original's powf) and the pure
// reimplementation checks it (row native_display_curve with the largest difference; above 1e-6 is a defect).
// I-0138 tranche 5b (Thomas, 2026-11-14 12:05): an HDR10 chain no longer takes the option (tranche 5 applied it there as a power):
// the paper white is its setting there, and the game's Brightness row is locked while the chain is HDR10 (02b).
DisplayCurve NativeDisplayCurve() {
    if(!native_display_source)return nullptr;   // review: no curve work without the pass (no-gamma.flag, a refused blob)
    static const int override_level=[]{int level=-1;
        if(const auto flag=ModuleSibling(L"brightness-override.flag");!flag.empty()){
            if(FILE* file{};!_wfopen_s(&file,flag.c_str(),L"rb") && file){int value{};if(fscanf_s(file,"%d",&value)==1 && value>=0 && value<=10)level=value;fclose(file);}}
        return level;}();
    std::array<float,6> values=DisplayCurveIdentity;
    {const std::lock_guard<std::mutex> lock(colour_settings_lock);if(native_colour_settings[0]>0.f)values=native_colour_settings;}
    if(override_level>=0){const float dst=2.2f+.5f*(float(override_level)-5.f)/5.f;values[3]=values[4]=values[5]=dst;}
    for(const auto value:values)if(!std::isfinite(value) || value<=0.f)values=DisplayCurveIdentity;
    static std::array<float,6> last{};static DisplayCurve curve;static uint64_t generation{};
    if(curve && values==last)return curve;
    auto bytes=std::make_shared<std::vector<uint8_t>>(DisplayCurveBytes);
    reinterpret_cast<void(__fastcall*)(void*,float,float,float,float,float,float)>(image+bootstrap::display_curve_generator_rva)(
        bytes->data(),values[0],values[1],values[2],values[3],values[4],values[5]);
    // Review: the engine's output is used only when every entry is finite, in [0, 1] and within 1e-6 of the pure curve;
    // otherwise the pure curve is (a wrong signature assumption would otherwise corrupt the whole image).
    std::vector<uint8_t> pure(DisplayCurveBytes);double largest{};bool valid=true;
    if(!GenerateDisplayCurve(values,std::span<uint8_t,DisplayCurveBytes>(pure.data(),DisplayCurveBytes)))return curve;
    for(size_t at=0;at<DisplayCurveBytes;at+=4){float a{},b{};std::memcpy(&a,bytes->data()+at,4);std::memcpy(&b,pure.data()+at,4);
        if(!std::isfinite(a) || a<0.f || a>1.f){valid=false;continue;}largest=std::max(largest,double(std::fabs(a-b)));}
    if(!valid || largest>1e-6){*bytes=pure;valid=false;}
    last=values;curve=std::move(bytes);++generation;
    static LONG64 rows{};if(InterlockedIncrement64(&rows)<=32 || !valid){char row[352]{};
        sprintf_s(row,"{\"event\":\"native_display_curve\",\"generation\":%llu,\"values\":[%g,%g,%g,%g,%g,%g],\"override\":%d,\"largest_difference\":%g,\"source\":\"%s\"}\n",
            static_cast<unsigned long long>(generation),values[0],values[1],values[2],values[3],values[4],values[5],override_level,largest,valid?"engine":"pure_fallback");Log(row);}
    return curve;
}
// 1118: material family key of a mesh (armour family inventory 1113: shader, blend, alpha ref).
template<class Programs> std::string NativeFamilyKey(const Programs& programs) {
    std::string key;for(const auto ch:programs.program_path)key.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
    if(programs.policy)key+=std::string(programs.policy->transparent?"|t|":"|o|")+std::to_string(programs.policy->alpha_ref);
    return key;
}
// I-0120 B2, C3: the 1068 summary (LogNativeAsmSummary) logs at most 40 rows; its feeders (the pass gates here, the part tally in
// PrepareNativeModelFrame) build their data only while it can still log. CapOpen: a relaxed read before a capped witness's increment.
std::atomic<LONG64> native_asm_summary_logged{};
bool NativeAsmSummaryOpen() noexcept {return native_asm_summary_logged.load(std::memory_order_relaxed)<40;}
bool CapOpen(LONG64& counter,LONG64 cap) noexcept {return std::atomic_ref<LONG64>(counter).load(std::memory_order_relaxed)<cap;}
// I-0071 point 3b (ER A, the entity Draw jobs): what the pass rows of some models give, kept apart per worker and merged into
// the scene afterwards (rows and part gates in the serial order, counters summed).
struct NativePassSink {
    frame_store::Array<NativeSelectedScene::PreparedMesh>* rows{};
    std::vector<std::pair<int32_t,uint8_t>>* asm_gates{};
    std::array<uint32_t,32> refusals{};uint32_t eligible_meshes{},unavailable_preparations{},no_cast_meshes{};
    std::map<std::string,std::map<int,uint32_t>> asm_family_gates;
    const NativeModelLayout::Programs* gate_programs{};   // m45 frame cost: the gated mesh's family key is made only when a gate counts it
    uint32_t census_pass{};
    // I-0091: emptied for another collect without allocating (a default-constructed map allocates its head on MSVC).
    void Reset() {refusals={};eligible_meshes=unavailable_preparations=no_cast_meshes=0;asm_family_gates.clear();gate_programs=nullptr;census_pass=0;}
};
// `parallel` (the main collect): the models' rows on the admission pool, in chunks of models taken by ticket, each chunk's
// rows kept per engine pass and merged pass by pass in chunk order, which is the order one thread gives (pass, then model).
void PrepareNativeSelectedPasses(NativeSelectedScene& scene,bool parallel=false) {
    static const bool census_flag=ModuleFlag(L"light-observe.flag");   // the asm gate rows (I-0120 C3)
    const bool asm_gate_rows=census_flag || NativeAsmSummaryOpen();   // I-0120 C3: read once per collect (the serial re-check below sees the same)
    // m50 sky (WAVE1-LAST-EFFECTS-STATIC-PREP 3.4): once, at the 3000th preparation, every admitted model's outcomes by name: per
    // engine pass the gate codes that refused it and the meshes prepared (code 100).
    static LONG64 outcome_calls{};const bool outcome_census=InterlockedIncrement64(&outcome_calls)==3000;
    std::map<std::string,std::set<std::pair<uint32_t,uint32_t>>> outcomes;
    const auto note_outcome=[&](const NativePassSink& sink,size_t model_index,uint32_t code){if(outcome_census)outcomes[NativeModelDataName(scene.models[model_index].model)].emplace(sink.census_pass,code);};
    // Fable review (the Crucifixion Woods cross): with "!name" in hide-texture.txt, every pass-row refusal of that model, once per
    // (pass, code, family), with the gate code (10 + the pass decision reason) and the mesh's family key.
    const auto named_fragment=chair_hunt::Fragment();const bool named_gates=named_fragment.size()>1 && named_fragment[0]==L'!';
    const auto gate=[&](NativePassSink& sink,size_t model_index,size_t code){++sink.refusals[code];note_outcome(sink,model_index,static_cast<uint32_t>(code));
        const auto* gate_programs=sink.gate_programs;const auto census_pass=sink.census_pass;
        if(named_gates && chair_hunt::Matches(named_fragment,scene.models[model_index].model)){static std::mutex lock;static std::set<std::tuple<uint32_t,size_t,std::string>> seen;
            const std::string family=gate_programs?NativeFamilyKey(*gate_programs):std::string();bool fresh{};
            {const std::lock_guard held(lock);fresh=seen.size()<96 && seen.emplace(census_pass,code,family).second;}
            if(fresh){const std::string row="{\"event\":\"native_named_gate\",\"model\":\""+NativeModelDataName(scene.models[model_index].model)+"\",\"engine_pass\":"+std::to_string(census_pass)+
                ",\"code\":"+std::to_string(code)+",\"family\":\""+family.substr(0,140)+"\"}\n";Log(row.c_str());}}
        if(asm_gate_rows && scene.models[model_index].asm_part>=0){sink.asm_gates->push_back({scene.models[model_index].asm_part,static_cast<uint8_t>(code)});
            if(gate_programs)++sink.asm_family_gates[NativeFamilyKey(*gate_programs)][static_cast<int>(code)];}};
    if(!scene.simulation_time || !scene.global_variant_enabled || !scene.category_scalar || !scene.split_engine5){++scene.refusals[0];return;}
    // Each entity/fade/pass invocation owns one fresh preparation scope.
    // Physical row order and late failure inheritance follow63CB0/140680.
    // 57A20 also adds the forward row (0x29, preparation 0, class mask 0x12): shader pass 0, Fwd (Reference1114).
    // 1181: and, for a model with +224 bit 12 (a forward material), the row (0x1F, preparation 0, class mask 0x80) that draws
    // the non-transparent forward meshes (alpha-tested hair), ahead of 0x29 (MENUS-MAP 57A20 rows).
    // 1186: and, last, the velocity row (0x34, preparation 5, class mask 0xFFFF; Velo) for a draw whose velocity byte is set,
    // while the main view has velocity inputs (quality option +0x21; I-0067 VELOCITY-SKINNING §7).
    // WATER-TESS-WAVE1-READY WT-1: and the water rows 0x24 (WatDpt) and 0x27 (the surface) after the 0x1F rows (the GPU order of 1.4).
    constexpr std::array<uint32_t,10> passes{5u,6u,7u,0x1Fu,0x24u,0x27u,0x21u,0x22u,0x29u,0x34u};   // I-0131 R10a: the cascade passes' row (1) is removed
    const auto rows_of=[&](NativePassSink& sink,const uint32_t engine_pass,const size_t model_index){
        const auto& model=scene.models[model_index];sink.census_pass=engine_pass;
        // I-0113 tranche 10d (57A20:550-569 for category 2, 657-720 for 0x29): a draw whose descriptor's byte +0x10 is set (the hair)
        // records at 0x29 a depth pre-pass right before its colour row (keys param_7 + and - a step): 5C0E0 / 5C060 with
        // preparation 1 (5A120: shader pass 2, Dpt; args+0x200 bits 11 and 19: no colour written, the depth by Begin), class mask
        // 2 for category 2 and 0xFFFF for 0x29; here the model's pre-pass rows, then its colour rows. Category 0x21 (an extra
        // row of pass 4, 57A20:628-660) is not transcribed.
        const bool with_prepass=engine_pass==0x29 && model.fade_prepass && !model.effect && model.ordinary_category &&
            (*model.ordinary_category==2 || *model.ordinary_category==0x29);
        if(engine_pass==0x34 && (!scene.velocity || !scene.velocity->rows || !model.velocity_rows))return;
        // 1100: 57A20 adds the engine7 row (preparation 3, material class 0x100: SAT_Equip armour) only
        // for a model with +224 bit 15. Explicit open policy: drawn with the engine5 G-buffer state.
        if(engine_pass==7){uint32_t flags224{};const bool read=model.material_flags?(flags224=*model.material_flags,true):Read(model.model+0x224,&flags224,4);
            // 1101 witness: the +220/+224 words of character-part models at the engine7 row decision.
            if(model.asm_part>=0){static LONG64 row_samples{};if(InterlockedIncrement64(&row_samples)<=48){uint32_t flags220{};Read(model.model+0x220,&flags220,4);
                char row[256]{};sprintf_s(row,"{\"event\":\"native_asm_engine7_row\",\"part\":%d,\"model\":%llu,\"read\":%s,\"flags220\":%u,\"flags224\":%u}\n",
                    model.asm_part,static_cast<unsigned long long>(model.model),read?"true":"false",flags220,flags224);Log(row);}}
            // L0.1 (NPC-FAMILIES-STATIC §7): the same words for each SprjModelDrawEntity model (pseudo part 1000), once per model.
            if(model.asm_part==1000){static std::mutex lock;static std::set<uintptr_t> seen;bool fresh{};
                {const std::lock_guard hold(lock);fresh=seen.size()<256 && seen.insert(model.model).second;}
                if(fresh){uint32_t flags220{};if(model.material_mask)flags220=*model.material_mask;else Read(model.model+0x220,&flags220,4);char row[256]{};
                    sprintf_s(row,"{\"event\":\"native_character_engine7_row\",\"name\":\"%s\",\"model\":%llu,\"read\":%s,\"flags220\":%u,\"flags224\":%u}\n",
                        NativeModelDataName(model.model).c_str(),static_cast<unsigned long long>(model.model),read?"true":"false",flags220,flags224);Log(row);}}
            if(!read || !(flags224>>15&1))return;}
        if(engine_pass==0x1F){uint32_t flags224{};const bool read=model.material_flags?(flags224=*model.material_flags,true):Read(model.model+0x224,&flags224,4);
            if(!read || !(flags224>>12&1))return;}
        if(!model.preparation_flags){gate(sink,model_index,1);return;}
        if(!model.shader_capabilities){gate(sink,model_index,2);return;}
        if(!model.material_mask){gate(sink,model_index,3);return;}
        if(!model.ordinary_category){gate(sink,model_index,4);return;}
        // P8 (PHANTOMS-SIGNS-NATIVE §5): category 2 takes the ordinary rows; 0x21 the phantom rows 0x21 and 0x22 (the latter
        // only with +224 bit 15); 0x29 the generic forward row 0x29 alone.
        {const auto category=*model.ordinary_category;
            if(category==2?(engine_pass==0x21 || engine_pass==0x22):category==0x21?(engine_pass!=0x21 && engine_pass!=0x22):
               category==0x29?engine_pass!=0x29:false)return;
            if(engine_pass==0x22 && model.effect)return;   // I-0113: a model particle of pass 0x21 draws once, in its pass
            if(engine_pass==0x22){uint32_t flags224{};const bool read=model.material_flags?(flags224=*model.material_flags,true):Read(model.model+0x224,&flags224,4);
                if(!read || !(flags224>>15&1))return;}}
        // 57A20: a split model gets two engine5 rows, class masks 1 then 4.
        const bool split=engine_pass==5 && *scene.split_engine5 && (*model.material_mask&4);
        for(const bool prepass:with_prepass?std::initializer_list<bool>{true,false}:std::initializer_list<bool>{false})
        for(const uint32_t row_class_mask:split?std::initializer_list<uint32_t>{1u,4u}:std::initializer_list<uint32_t>{0u})
        for(uint32_t fade_index=0;fade_index<model.geometry_fades.count;++fade_index){
            const auto& fade=model.geometry_fades.intents[fade_index];if(fade.variant<0)continue;
            ds3content::TextureUvScope scope(model.preparation_flags->skip_kind8,model.preparation_flags->special_kind5);
            ds3content::ModelPreparationPairState pairs;
            std::shared_ptr<const ds3content::TextureScrollVersion> active_scroll;
            for(size_t mesh_index=0;mesh_index<model.meshes.size();++mesh_index){
                const auto& mesh=model.meshes[mesh_index];
                sink.gate_programs=model.asm_part>=0?mesh.programs.get():nullptr;
                if(!mesh.material_class){gate(sink,model_index,5);continue;}
                if(!mesh.material_masks){gate(sink,model_index,6);continue;}
                if(!mesh.base_deformation){gate(sink,model_index,7);continue;}
                if(!mesh.programs){gate(sink,model_index,8);continue;}
                if(!mesh.programs->policy){gate(sink,model_index,9);continue;}
                // WT-1 (1400575C0): a material with g_GXFT_PlanarReflection (policy bit 4) or g_GXFT_Refraction (bit 5) is a kind-0x27
                // draw: the rows 0x24 and 0x27 and no other (57A20:626-685).
                if(((mesh.programs->policy->flags&0x30)!=0)!=(engine_pass==0x24 || engine_pass==0x27))continue;
                ds3content::OrdinaryModelPassInput input;
                const bool water_depth_row=engine_pass==0x24,forward_row=engine_pass==0x29 || engine_pass==0x1F || engine_pass==0x21 || engine_pass==0x22 || engine_pass==0x24 || engine_pass==0x27,velocity_row=engine_pass==0x34,shadow_row=engine_pass==1;
                input.family=shadow_row?ds3content::NativeModelPassFamily::OrdinaryShadow:velocity_row?ds3content::NativeModelPassFamily::OrdinaryVelocity:
                    forward_row?ds3content::NativeModelPassFamily::OrdinaryForward:ds3content::NativeModelPassFamily::OrdinaryDeferred;input.category=*model.ordinary_category;
                input.engine_pass=engine_pass;input.preparation=shadow_row?2:velocity_row?5:water_depth_row?6:prepass?1:forward_row?0:3;input.shader_pass=shadow_row || prepass?2:velocity_row?1:water_depth_row?12:forward_row?0:6;input.required_mesh_mask=1;
                input.requested_lod=static_cast<uint32_t>(fade.variant);input.model_lod_mask=model.lod_mask;
                if(split){input.flags=static_cast<uint32_t>(ds3content::NativeModelPassFlag::SplitEngine5);input.row_class_mask=row_class_mask;}
                // W5e TS-1 (W0a-6 flat fallback): the parsed GX04/GX90 deformation stays flat until native-tess.flag releases it
                // (W7 TS-2b: with the flag, a deformation-3 row takes its `_PN` recipe when that resolved, any other the flat recipe).
                if(!NativeTessellationEnabled())input.flags|=static_cast<uint32_t>(ds3content::NativeModelPassFlag::DisableDeformation);
                if(engine_pass!=7)input.model_material_mask=*model.material_mask;input.mesh_flags=mesh.current.flags;input.mesh_lod_mask=mesh.current.lod_mask;
                input.material_class=*mesh.material_class;input.material_secondary=mesh.material_masks->secondary;
                input.material_capabilities_first=mesh.material_masks->first;input.material_capabilities_second=mesh.material_masks->second;
                input.group_capabilities_first=(*model.shader_capabilities)[0];input.group_capabilities_second=(*model.shader_capabilities)[1];
                input.layout_type=mesh.current.layout_type;input.base_deformation=*mesh.base_deformation;
                input.prefer_channel=mesh.programs->prefer_channel;
                // The fade's dither byte (I-0113 tranches 19, 22): 5C0E0:101-113 takes the group's +0x89 for 57A20's casters' row of the
                // views 1..0x38 and +0x88 for every other; the native +0x88 for every row (the raster casters give way to the traced
                // shadows, D-0250: the casters' rule is R's).
                input.pixel={mesh.programs->policy->alpha_ref,*scene.global_variant_enabled,
                    fade.fade_enabled && fade.opacity<*scene.category_scalar,scene.pixel_policy};
                const auto decision=ds3content::EvaluateOrdinaryModelPass(input);
                // W7 witness native_tess_candidate: a row of a mesh whose material asks for a deformation (pass, LOD, outcome).
                if(input.base_deformation){static LONG64 candidates{};if(InterlockedIncrement64(&candidates)<=24){char row[360]{};
                    sprintf_s(row,"{\"event\":\"native_tess_candidate\",\"base\":%u,\"effective\":%u,\"engine_pass\":%u,\"lod\":%u,\"status\":%u,\"reason\":%u,\"flags\":%u,\"family\":\"%.120s\"}\n",
                        input.base_deformation,decision.effective_deformation,engine_pass,input.requested_lod,static_cast<unsigned>(decision.status),static_cast<unsigned>(decision.reason),
                        input.flags,NativeFamilyKey(*mesh.programs).c_str());Log(row);}}
                // 1098 witness: a character-part mesh refused by material class (armour parts 2-6 of the player).
                if(decision.status!=ds3content::NativeModelPassStatus::Eligible && model.asm_part>=0 &&
                   decision.reason==ds3content::NativeModelPassReason::MaterialClass){static LONG64 class_samples{};
                    if(CapOpen(class_samples,40) && InterlockedIncrement64(&class_samples)<=40){std::string family;   // I-0120 C3: CapOpen first
                        for(const auto ch:mesh.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                        const auto& policy=*mesh.programs->policy;
                        char row[512]{};sprintf_s(row,"{\"event\":\"native_asm_part_class_gate\",\"part\":%d,\"mesh\":%u,\"engine_pass\":%u,\"split\":%s,\"row_class_mask\":%u,\"class\":%u,\"special\":%s,\"policy_flags\":%u,\"forward\":%s,\"transparent\":%s,\"alpha_ref\":%u,\"family\":\"%.120s\"}\n",
                            model.asm_part,mesh.current.ordinal,engine_pass,split?"true":"false",row_class_mask,*mesh.material_class,
                            mesh.current_special && *mesh.current_special?"true":"false",policy.flags,policy.forward?"true":"false",policy.transparent?"true":"false",policy.alpha_ref,family.c_str());Log(row);}}
                // 1176 witness: a mesh refused by the capability masks (Load Game portraits lose their hair natively).
                if(decision.status!=ds3content::NativeModelPassStatus::Eligible && decision.reason==ds3content::NativeModelPassReason::Capabilities){
                    // 1181: hair meshes counted apart (the first 40 rows are taken by the portraits' masked body meshes).
                    const bool hair=mesh.programs->program_path.find(L"Hair")!=std::wstring::npos;
                    static LONG64 capability_samples[2]{};if(CapOpen(capability_samples[hair?1:0],40) && InterlockedIncrement64(&capability_samples[hair?1:0])<=40){std::string family;   // I-0120 C3: CapOpen first
                        for(const auto ch:mesh.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                        char row[512]{};sprintf_s(row,"{\"event\":\"native_capability_gate\",\"part\":%d,\"model\":%u,\"mesh\":%u,\"engine_pass\":%u,\"material\":[%llu,%llu],\"group\":[%llu,%llu],\"family\":\"%.140s\"}\n",
                            model.asm_part,static_cast<uint32_t>(model_index),mesh.current.ordinal,engine_pass,static_cast<unsigned long long>(input.material_capabilities_first),static_cast<unsigned long long>(input.material_capabilities_second),
                            static_cast<unsigned long long>(input.group_capabilities_first),static_cast<unsigned long long>(input.group_capabilities_second),family.c_str());Log(row);}}
                // 1181 witness: any refusal of a hair-family mesh in a menu view (pass, reason, model, mesh).
                if(decision.status!=ds3content::NativeModelPassStatus::Eligible && model.asm_part<0 && mesh.programs->hair_family){
                    static LONG64 hair_refusals{};if(CapOpen(hair_refusals,60) && InterlockedIncrement64(&hair_refusals)<=60){char row[200]{};   // I-0120 C3: CapOpen first
                        sprintf_s(row,"{\"event\":\"native_offscreen_hair_refused\",\"model\":%u,\"mesh\":%u,\"engine_pass\":%u,\"reason\":%u,\"status\":%u,\"model_mask\":%lld,\"class\":%u,\"secondary\":%u}\n",
                        static_cast<uint32_t>(model_index),mesh.current.ordinal,engine_pass,static_cast<uint32_t>(decision.reason),static_cast<uint32_t>(decision.status),
                        input.model_material_mask?static_cast<long long>(*input.model_material_mask):-1ll,input.material_class,input.material_secondary);Log(row);}}
                // m50 sky (light-observe.flag): the first 24 map-piece meshes refused at the forward rows for their material class.
                {static const bool class_diagnostics=ModuleFlag(L"light-observe.flag");
                    if(class_diagnostics && decision.status!=ds3content::NativeModelPassStatus::Eligible && decision.reason==ds3content::NativeModelPassReason::MaterialClass &&
                       model.asm_part<0 && (engine_pass==0x29 || engine_pass==0x1F) && *mesh.material_class!=1){static std::mutex forward_class_lock;static std::set<std::tuple<uintptr_t,uint32_t,uint32_t>> forward_class_seen;bool fresh{};
                        {const std::lock_guard hold(forward_class_lock);fresh=forward_class_seen.size()<48 && forward_class_seen.emplace(model.model,mesh.current.ordinal,engine_pass).second;}if(fresh){
                        std::string family,name;for(const auto ch:mesh.programs->program_path)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                        for(const auto ch:NativeModelDataName(model.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');const auto& policy=*mesh.programs->policy;
                        char row[512]{};sprintf_s(row,"{\"event\":\"native_forward_class_gate\",\"model\":\"%.40s\",\"mesh\":%u,\"engine_pass\":%u,\"class\":%u,\"secondary\":%u,\"policy_flags\":%u,\"forward\":%s,\"transparent\":%s,\"blend\":%u,\"category\":%d,\"family\":\"%.100s\"}\n",
                            name.c_str(),mesh.current.ordinal,engine_pass,*mesh.material_class,input.material_secondary,policy.flags,policy.forward?"true":"false",policy.transparent?"true":"false",policy.blend_mode,
                            model.ordinary_category?int(*model.ordinary_category):-1,family.c_str());Log(row);}}}
                if(decision.status!=ds3content::NativeModelPassStatus::Eligible){gate(sink,model_index,10+std::min<uint32_t>(static_cast<uint32_t>(decision.reason),13));continue;}
                ++sink.eligible_meshes;
                const ds3content::ModelPreparationPair pair{mesh.material.get(),mesh.current.layout_type};
                std::shared_ptr<const ds3content::TextureScrollVersion> scroll=active_scroll;
                if(pairs.Begin(pair)){
                    active_scroll.reset();
                    if(!mesh.material->scroll){++sink.unavailable_preparations;gate(sink,model_index,24);continue;}
                    scroll=mesh.material->scroll->Update(scene.simulation_time->generation,scene.simulation_time->now,scene.uv_rate);
                    if(!scroll || !ds3content::MergeTextureScrollContribution(scope,scroll->offsets)){
                        ++sink.unavailable_preparations;gate(sink,model_index,25);continue;
                    }
                    active_scroll=scroll;
                }
                //230610 and contribution merge precede the late programme
                //refusal. A failed preparation never erases scope offsets.
                if(!decision.intent || decision.intent->instanced ||
                   mesh.layout->instanced || !mesh.programs->texture_bindings_available){++sink.unavailable_preparations;
                    gate(sink,model_index,!decision.intent?26:(decision.intent->instanced || mesh.layout->instanced)?28:29);continue;}
                // W7 TS-2b (W0a-6): a non-zero deformation (only with native-tess.flag) takes the `_PN` recipe of its pass when
                // it resolved with both stages' texture selections ready; any other row (another deformation, another pass, an
                // unresolved recipe) is drawn flat by the recipe of its pass, counted in native_tess_rows.
                uint32_t tess_recipe=uint32_t(native_recipe_count);
                if(decision.intent->deformation){
                    const auto candidate=decision.intent->deformation==3?NativeTessRecipe(decision.intent->shader_pass,decision.intent->pixel_variant):uint32_t(native_recipe_count);
                    const char* outcome="pn";int status=-1;
                    if(candidate>=native_recipe_count)outcome="pass";
                    else if(mesh.programs->recipes[candidate].status!=ds3content::ModelProgramStatus::Resolved){outcome="recipe";status=static_cast<int>(mesh.programs->recipes[candidate].status);}
                    else if(!mesh.programs->pn_triangles)outcome="block";
                    else if(mesh.shader_textures[candidate][0].status!=ds3content::ModelTextureStatus::Ready ||
                            mesh.shader_textures[candidate][1].status!=ds3content::ModelTextureStatus::Ready){outcome="textures";
                        status=static_cast<int>(mesh.shader_textures[candidate][0].status)*16+static_cast<int>(mesh.shader_textures[candidate][1].status);}
                    else if(!mesh.layout->vertex_elements_available || !mesh.programs->interfaces[candidate][0] ||
                            !NativeLayoutFeeds(*mesh.programs->interfaces[candidate][0],mesh.layout->vertex_elements))outcome="layout";
                    else tess_recipe=candidate;
                    if(NativeTessRowLogged(outcome))NoteNativeTessRow(decision.intent->deformation,decision.intent->shader_pass,outcome,status,NativeFamilyKey(*mesh.programs));}
                // C5 (MENUS-MAP 75-76): a forward draw whose state byte +0x87 (the PhantomParam id, group +0x87) is set takes the
                // alternate set (140140850: mat+0x78 when DC+0x425): <family>_Phantom_Fwd[_[A]] when that programme exists.
                auto recipe=NativeRecipe(decision.intent->shader_pass,decision.intent->pixel_variant);
                if(tess_recipe<native_recipe_count)recipe=tess_recipe;
                if(forward_row && !prepass && decision.intent->pixel_variant<2){BYTE phantom{};
                    const auto alternate=NativeRecipe(native_phantom_forward_pass,decision.intent->pixel_variant);
                    // Explicit policy (pre-merge review): the alternate is taken only when its texture selection is ready for both
                    // stages; otherwise the draw keeps the normal Fwd programme rather than being dropped, with a witness.
                    if((model.phantom || (Read(model.group+0x87,&phantom,1) && phantom)) && mesh.programs->recipes[alternate].status==ds3content::ModelProgramStatus::Resolved){
                        const bool ready=mesh.shader_textures[alternate][0].status==ds3content::ModelTextureStatus::Ready &&
                            mesh.shader_textures[alternate][1].status==ds3content::ModelTextureStatus::Ready;
                        if(ready)recipe=alternate;
                        else{static LONG64 fallback_samples{};if(InterlockedIncrement64(&fallback_samples)<=8){char row[160]{};
                            sprintf_s(row,"{\"event\":\"native_phantom_forward_fallback\",\"vertex_status\":%u,\"pixel_status\":%u}\n",
                                static_cast<unsigned>(mesh.shader_textures[alternate][0].status),static_cast<unsigned>(mesh.shader_textures[alternate][1].status));Log(row);}}}
                    // Summoned phantoms (Sword Master skin, Reference1921 against Native1920): the first 96 forward rows of ChrAsm parts
                    // by (model, mesh, pass): the part's phantom state, the live group bytes +0x84..+0x87 and whether the alternate was taken.
                    static const bool phantom_witness=ModuleFlag(L"light-observe.flag");
                    // Phantom parts only (the host's own parts would fill the 96 first).
                    if(phantom_witness && model.asm_part>=0 && model.phantom){static std::mutex phantom_row_lock;static std::set<std::tuple<uintptr_t,uint32_t,uint32_t>> phantom_row_seen;
                        static std::atomic<bool> phantom_row_full{};bool fresh{};
                        if(!phantom_row_full.load(std::memory_order_relaxed)){const std::lock_guard hold(phantom_row_lock);
                            fresh=phantom_row_seen.size()<96 && phantom_row_seen.emplace(model.model,mesh.current.ordinal,engine_pass).second;if(phantom_row_seen.size()>=96)phantom_row_full.store(true);}
                        if(fresh){std::array<uint8_t,4> bytes{};Read(model.group+0x84,bytes.data(),4);float group_alpha{};Read(model.group+0xDC,&group_alpha,4);std::string family;
                            for(const auto ch:mesh.programs->program_path)if(family.size()<90)family.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                            char row[480]{};sprintf_s(row,"{\"event\":\"native_phantom_part_row\",\"asm_part\":%d,\"model\":\"%.40s\",\"mesh\":%u,\"engine_pass\":%u,\"category\":%u,\"phantom\":%s,\"kind\":%u,\"group84\":[%u,%u,%u,%u],\"alternate_resolved\":%s,\"alternate\":%s,\"fade\":%g,\"group_alpha\":%g,\"family\":\"%s\"}\n",
                                model.asm_part,NativeModelDataName(model.model).c_str(),mesh.current.ordinal,engine_pass,model.ordinary_category?*model.ordinary_category:0u,model.phantom?"true":"false",model.phantom_kind,
                                bytes[0],bytes[1],bytes[2],bytes[3],mesh.programs->recipes[alternate].status==ds3content::ModelProgramStatus::Resolved?"true":"false",recipe==alternate?"true":"false",model.phantom_fade,group_alpha,family.c_str());Log(row);}}}
                if(recipe>=native_recipe_count || mesh.programs->recipes[recipe].status!=ds3content::ModelProgramStatus::Resolved){
                    // 1028 witness: the unresolved recipe and the programmes it chose from.
                    // m50 sky (WAVE1-LAST-EFFECTS-STATIC-PREP 3.4): one row per family path and recipe (at most 32), with the model's name;
                    // the programme list keeps the forward ones first (the list was cut before them).
                    static std::mutex recipe_lock;static std::set<std::pair<std::wstring,size_t>> recipe_seen;static std::atomic<bool> recipe_full{};bool first_seen=false;
                    if(!recipe_full.load(std::memory_order_relaxed)){const std::lock_guard hold(recipe_lock);   // review: no lock once the 32 rows are written
                        if(recipe_seen.size()<32)first_seen=recipe_seen.emplace(mesh.programs->program_path,recipe).second;else recipe_full=true;}
                    if(first_seen){std::string names,others;
                        for(const auto& program:mesh.programs->effect->program_names){std::string name;
                            for(const auto ch:program.name)name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                            auto& list=name.find("Fwd")!=std::string::npos?names:others;if(list.size()<1400)list+=(list.empty()?"":",")+name;}
                        names+=(names.empty() || others.empty()?"":",")+others;
                        {std::string model_name;for(const auto ch:NativeModelDataName(model.model))model_name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');names=model_name+"|"+names;}
                        std::string path;for(const auto ch:mesh.programs->program_path)path.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
                        std::string row="{\"event\":\"native_recipe_unresolved\",\"recipe\":"+std::to_string(recipe)+",\"status\":"+
                            std::to_string(recipe<native_recipe_count?static_cast<int>(mesh.programs->recipes[recipe].status):-1)+",\"layout_type\":"+std::to_string(mesh.current.layout_type)+
                            ",\"instanced\":"+std::to_string(mesh.layout->instanced)+",\"prefer_channel\":"+std::to_string(mesh.programs->prefer_channel)+
                            ",\"path\":\""+path.substr(0,300)+"\",\"programmes\":\""+names+"\"}\n";Log(row.c_str());}
                    ++sink.unavailable_preparations;gate(sink,model_index,30);continue;
                }
                ds3content::ModelTextureTiling tiling;
                //1A9530's explicit fresh ordinary tiling register1. Other
                //family register setters require their own typed inputs.
                const ds3content::ModelTextureTilingInputs tiling_input{1,scope.skip_kind8,scope.special_kind5,scope.offsets};
                if(!ds3content::ProduceModelTextureTiling(mesh.programs->texture_bindings,tiling_input,tiling)){
                    ++sink.unavailable_preparations;gate(sink,model_index,31);continue;
                }
                if(!pairs.Current() && !pairs.RememberSuccess(pair))Refuse("native_model_preparation_pair_commit");
                sink.rows->emplace_back(model_index,mesh_index,fade_index,engine_pass,recipe,std::move(tiling),std::move(scroll));note_outcome(sink,model_index,100);   // I-0091: built in its place
                for(size_t stage=0;stage<2;++stage)sink.rows->back().instances[stage]=&NativeMeshInstanceBlock(model,mesh,fade_index,recipe,stage);   // I-0074
            }
            sink.gate_programs=nullptr;
        }
    };
    const auto merge=[&](const NativePassSink& sink){
        for(size_t i=0;i<scene.refusals.size();++i)scene.refusals[i]+=sink.refusals[i];
        scene.eligible_meshes+=sink.eligible_meshes;scene.unavailable_preparations+=sink.unavailable_preparations;scene.no_cast_meshes+=sink.no_cast_meshes;
        for(const auto& [family,codes]:sink.asm_family_gates)for(const auto& [code,count]:codes)scene.asm_family_gates[family][code]+=count;};
    auto& pool=AdmissionPool();constexpr size_t models_per_chunk=32;
    if(!parallel || outcome_census || scene.models.size()<2*models_per_chunk || pool.Workers()<2){
        NativePassSink sink;sink.rows=&scene.prepared_meshes;sink.asm_gates=&scene.asm_gates;
        scene.prepared_meshes.reserve(scene.prepared_meshes.size()+2*scene.models.size());   // I-0091: about two rows a model (no regrowth chain)
        for(const auto engine_pass:passes)for(size_t model_index=0;model_index<scene.models.size();++model_index)rows_of(sink,engine_pass,model_index);
        merge(sink);
    }
    else{
        struct Chunk {NativePassSink sink;std::array<frame_store::Array<NativeSelectedScene::PreparedMesh>,std::tuple_size_v<decltype(passes)>> rows;std::array<std::vector<std::pair<int32_t,uint8_t>>,std::tuple_size_v<decltype(passes)>> gates;};
        // I-0078: chunks of 32 models while many remain, then smaller ones down to 4 (fixed bounds, so the join's order stays
        // the serial one). I-0091: the chunks and their rows' storage kept by the collecting thread from one main collect to the
        // next (never destroyed, as the frame store: nothing is released at process exit), emptied here; each row built once in
        // its chunk and moved once into its place in the scene. (A reference: the workers reach the collecting thread's chunks.)
        std::vector<size_t> bounds{0};
        for(size_t at=0;at<scene.models.size();){at=(std::min)(scene.models.size(),at+std::clamp<size_t>((scene.models.size()-at)/(2*pool.Workers()),4,models_per_chunk));bounds.push_back(at);}
        thread_local auto& kept_chunks=*new std::vector<Chunk>;if(kept_chunks.size()<bounds.size()-1)kept_chunks.resize(bounds.size()-1);
        const std::span<Chunk> chunks(kept_chunks.data(),bounds.size()-1);
        for(auto& part:chunks){part.sink.Reset();for(auto& rows:part.rows)rows.clear();for(auto& gates:part.gates)gates.clear();}
        std::atomic<size_t> ticket{};
        const size_t first_gate=scene.asm_gates.size();
        const std::function<void(size_t)> job=[&](size_t){
            for(size_t c;(c=ticket.fetch_add(1,std::memory_order_relaxed))<chunks.size();){auto& part=chunks[c];
                for(size_t p=0;p<passes.size();++p){part.sink.rows=&part.rows[p];part.sink.asm_gates=&part.gates[p];
                    for(size_t model_index=bounds[c];model_index<bounds[c+1];++model_index)rows_of(part.sink,passes[p],model_index);}}};
        pool.Run(job);
        // I-0077: the rows joined in the same order (pass, then chunk), each block moved into its place on the pool (i74-native-53:
        // the serial join of about 3,000 rows was half of this function on the engine thread).
        const size_t blocks=passes.size()*chunks.size();std::vector<size_t> at(blocks);size_t total=0;
        for(size_t p=0;p<passes.size();++p)for(size_t c=0;c<chunks.size();++c){at[p*chunks.size()+c]=total;total+=chunks[c].rows[p].size();
            scene.asm_gates.insert(scene.asm_gates.end(),chunks[c].gates[p].begin(),chunks[c].gates[p].end());}
        // I-0091: places nothing initialised before (the moves cannot throw), committed once every block is in.
        const size_t first_row=scene.prepared_meshes.size();
        NativeSelectedScene::PreparedMesh* const places=scene.prepared_meshes.Uninitialized(total);
        std::atomic<size_t> next_block{};
        const std::function<void(size_t)> place=[&](size_t){
            for(size_t k;(k=next_block.fetch_add(1,std::memory_order_relaxed))<blocks;){auto& rows=chunks[k%chunks.size()].rows[k/chunks.size()];
                std::uninitialized_move(rows.begin(),rows.end(),places+at[k]);}};
        pool.Run(place);
        scene.prepared_meshes.Commit(total);
        NativePassSink sum;for(const auto& part:chunks){for(size_t i=0;i<sum.refusals.size();++i)sum.refusals[i]+=part.sink.refusals[i];
            sum.eligible_meshes+=part.sink.eligible_meshes;sum.unavailable_preparations+=part.sink.unavailable_preparations;sum.no_cast_meshes+=part.sink.no_cast_meshes;}
        for(const auto& part:chunks)merge(part.sink);
        // Guard (record-revalidation.flag): every 64th parallel preparation made again on this thread and compared row by row
        // (the scroll versions are the same objects: TextureScrollOwner::Update returns its current version for one clock).
        static LONG64 parallel_calls{};static std::atomic<uint64_t> checks{},mismatches{};
        if(NativeRecordRevalidationPeriod()<512 && InterlockedIncrement64(&parallel_calls)%64==0){
            frame_store::Array<NativeSelectedScene::PreparedMesh> serial_rows;std::vector<std::pair<int32_t,uint8_t>> serial_gates;
            NativePassSink one;one.rows=&serial_rows;one.asm_gates=&serial_gates;
            for(const auto engine_pass:passes)for(size_t model_index=0;model_index<scene.models.size();++model_index)rows_of(one,engine_pass,model_index);
            const char* field=nullptr;
            if(serial_rows.size()!=scene.prepared_meshes.size()-first_row)field="rows";
            else for(size_t i=0;i<serial_rows.size() && !field;++i){const auto& a=serial_rows[i];const auto& b=scene.prepared_meshes[first_row+i];
                if(a.model!=b.model || a.mesh!=b.mesh || a.fade!=b.fade || a.engine_pass!=b.engine_pass || a.recipe!=b.recipe || a.instances!=b.instances)field="row";
                else if(a.tiling.buffer_register!=b.tiling.buffer_register || a.tiling.stages!=b.tiling.stages || !(a.tiling.values==b.tiling.values))field="tiling";
                else if(a.scroll!=b.scroll)field="scroll";}
            if(!field && (one.refusals!=sum.refusals || one.eligible_meshes!=sum.eligible_meshes || one.unavailable_preparations!=sum.unavailable_preparations ||
               one.no_cast_meshes!=sum.no_cast_meshes))field="counters";
            if(!field && !std::equal(serial_gates.begin(),serial_gates.end(),scene.asm_gates.begin()+first_gate,scene.asm_gates.end()))field="gates";
            const auto n=checks.fetch_add(1)+1;const auto bad=mismatches.fetch_add(field?1:0)+(field?1:0);
            if(field || n<=4 || n%64==0){char row[256]{};sprintf_s(row,"{\"event\":\"native_pass_rows_parallel_check\",\"checks\":%llu,\"mismatches\":%llu,\"rows\":%zu,\"chunks\":%zu,\"field\":\"%s\"}\n",
                static_cast<unsigned long long>(n),static_cast<unsigned long long>(bad),serial_rows.size(),chunks.size(),field?field:"");Log(row);}}
    }
    if(outcome_census){std::string chunk;size_t listed{};
        for(const auto& [name,cells]:outcomes){std::string clean;for(const auto ch:name)clean.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
            std::string list;for(const auto& [pass,code]:cells)list+=(list.empty()?"":",")+std::to_string(pass)+":"+std::to_string(code);
            chunk+=(chunk.empty()?"":",")+("[\""+clean+"\",\""+list+"\"]");
            if(++listed%50==0 || listed==outcomes.size()){const std::string row="{\"event\":\"native_model_outcomes\",\"to\":"+std::to_string(listed)+",\"models\":["+chunk+"]}\n";Log(row.c_str());chunk.clear();}}}
}
// 1186 velocity inputs of the main view (I-0068 VELOCITY-IMPLEMENTATION-CHECKLIST steps 1-3, 6.2). 57A20 records the 0x34
// row only while [[GXRS+0x5360]+0x21] (the MotionBlur quality option) holds; the view-projection of the previous selection
// of the same view is kept for VC_PrevMatrixWVP. A new view, the first selection after a reset, or a camera cut (more than
// 10 m or 60 degrees between two selections; explicit policy for the warps and loads that 13BFC0's first-frame flag covers)
// restarts it from the current one (zero camera motion).
// 1186: the MotionBlur quality row ([GXRS+0x5360]+0x20: pass byte, +0x21: object-row byte, +0x24: sample cap). 220A0 builds
// it (and 23790 refreshes it) from the quality table at GXRS+0x5358, row +0xA0 + 8 x level, level = GXRS+0x5330, which
// CED9A0 copies from the graphics options (the dword at [4796298]+0xA0+0x18, the fifth of the level words it reads from +0xA8).
// Levels 0-4: (0,0,0) (1,0,1) (1,1,1) (1,1,5) (1,1,8). O2: the row is the quality
// object's +0x20 in the scene's option snapshot (23790 copies that table row there), and the level its slot 4.
// W5a SW-1/SW-2 (SHADOWS-WAVE1-READY 2.1, 2.2): the shadow gates and state from the scene's own inputs: the quality object's
// shadow byte (+0x28), row (+0x2C..+0x3C) and Lighting byte (+0x44) from the scene's option snapshot, the view context's
// ShadowParam record R, 1B5720's sizes and clamps (shadow_state), the root's local-shadow mode byte +0x4E1A.
// I-0140 tranche 7 (the traced sun's true shadow source; R's readings of 2026-11-14 re-read on the listings, GhidraMCP): R is
// 88A10's: with the context's byte +0x3ED0 its record +0x90 as it is, else the ShadowParam records of the keys A +0x3FA8 and
// B +0x3FB0 (Banks::Shadow, 18C590's key rule) blended at the weight +0x3FB8 by DAF40 into the context's own record (kept from
// frame to frame); the sun is F90F0's direction from R's keys (ShadowSunDirection: the Shadow Light Id's light set, the blend of
// two, or the ShadowParam's Angle when the id is -1). Before this the native took the area of the MotionBlur layer's key
// +0x4008 with id 0 for both, wrong in a cutscene by construction.
// I-0141: where a cutscene's record names no light set, the traced sun takes light 0 of the light set at the view's own key (the
// cut's, which its characters carry; light_producers.h CutsceneShadowLightSet), only where that light is on (else the record's
// Angle stays), a departure from the original's horizontal shadow sun chosen for the one traced sun.
// SW-8: the gate words of the view context (row SL1, Reference1507: +0x7C = 3, +0x3EBC = 1, +0x2DA8 = 0 in game) are logged as
// the context holds them natively; the variant of 103510 stays open.
void CollectNativeShadowState(NativeSelectedScene& scene,uintptr_t view) {
    // I-0132 R11e: no-sun-shadow.flag removed (it left every frame without a traced sun: unserved, a fatal close).
    static LONG64 calls{};const auto n=InterlockedIncrement64(&calls);
    uintptr_t context{},root{};BYTE local_mode{},fixed_record{};float eps{},blend_eps{};
    {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
        const auto found=scene_view_cpu_owners.find(view);if(found!=scene_view_cpu_owners.end())context=found->second.context;}
    uint32_t keys[4]{};float weight{};
    const bool keyed=context && Read(context+0x3FA8,keys,16) && Read(context+0x3FB8,&weight,4);
    int32_t gate_words[1]{};uint8_t gate_bytes[2]{};
    if(context){Read(context+0x7C,&gate_words[0],4);Read(context+0x3EBC,&gate_bytes[0],1);Read(context+0x2DA8,&gate_bytes[1],1);Read(context+0x3ED0,&fixed_record,1);}
    if(Read(image+0x4961B50,&root,8) && root)Read(root+0x4E1A,&local_mode,1);
    Read(image+0x3D7ED70,&eps,4);Read(image+0x3D7ECE8,&blend_eps,4);
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    alignas(16) uint8_t record[0x94]{};bool recorded{};
    if(context && banks && (keyed || fixed_record)){
        {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
            const auto owner=scene_context_cpu_owners.find(context);
            if(owner!=scene_context_cpu_owners.end()){auto& kept=owner->second;
                if(!kept.shadow_record_ready){ds3light::ShadowDefaults(kept.shadow_record.data());kept.shadow_record_ready=true;}
                std::memcpy(record,kept.shadow_record.data(),sizeof record);recorded=true;}}
        // 88A10 with +0x3ED0: the context's record as it is, unblended. Natively that record is the kept copy (the engine's +0x90 is
        // never written in the native context: 843C0 and E00270 do not run), so it stands; the row says the byte was seen.
        if(recorded && fixed_record){static LONG64 rows{};if(InterlockedIncrement64(&rows)<=16)Log("{\"event\":\"native_shadow_fixed_record\",\"kept\":true}\n");}
        else if(recorded){alignas(16) uint8_t a[0x94]{},b[0x94]{};
            if(!banks->Shadow(keys[0],keys[1],a,sizeof a))ds3light::ShadowDefaults(a);   // 18C590's built-in record
            if(keys[2]==keys[0] && keys[3]==keys[1])std::memcpy(b,a,sizeof b);   // the same key (play: 888B0 sets A = B): one lookup
            else if(!banks->Shadow(keys[2],keys[3],b,sizeof b))ds3light::ShadowDefaults(b);
            ds3light::BlendShadowRecord(record,a,b,weight,blend_eps);
            const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
            if(const auto owner=scene_context_cpu_owners.find(context);owner!=scene_context_cpu_owners.end())std::memcpy(owner->second.shadow_record.data(),record,sizeof record);}}
    const bool quality=scene.options && scene.options->object_valid && scene.options->object.size()>=0x48;
    shadow::QualityRow row{};BYTE shadows_on=1,lighting{};
    if(quality){const auto* q=scene.options->object.data();shadows_on=q[0x28];lighting=q[0x44];
        std::memcpy(&row.num,q+0x2C,4);std::memcpy(&row.den,q+0x30,4);std::memcpy(&row.min,q+0x34,4);std::memcpy(&row.max,q+0x38,4);
        std::memcpy(&row.blur_add,q+0x3C,4);if(!row.den)row.den=1;}
    const auto sizes=recorded?shadow::ShadowSizes(record,quality?&row:nullptr,false,2):shadow::Sizes{};
    const auto state=recorded?shadow::ShadowClamps(record,eps):shadow::State{};
    int32_t filter{},resolution{};std::memcpy(&filter,record+0x50,4);std::memcpy(&resolution,record+0x90,4);
    float sun[4]{};
    const auto source=recorded?ds3light::ShadowSunDirection(*banks,record,keys,sun):ds3light::ShadowSunSource::Missing;   // I-0141: the view's keys, for a cutscene's record without a light set
    const bool sun_read=source!=ds3light::ShadowSunSource::Missing;
    uint32_t tags[4]{};std::memcpy(tags,record+0x20,16);float record_weight{};std::memcpy(&record_weight,record+0x30,4);
    NativeSelectedScene::Shadow produced;
    // I-0132 R11e (its step review's L2; R9c's rule, no DS3 option cuts an input of the rays): no longer gated on the Shadow Quality
    // row's size (a 0 there left the frame without a traced sun: unserved, then the fatal close); the words take the class edge then.
    if(recorded && sun_read){
        // I-0131 R10a: the four cascade fits (FUN_140239610), their caster volumes (FUN_14023C1E0), the atlas matrices, viewports,
        // rasterizer and filter plan are removed with the cascades; what stays is the cbSceneParam shadow words (SceneWords: the
        // shadowed sun pass reads +0x1D0 FC_ShadowColor), the sun and the quality gate. The random texture edge is 32, the value the
        // image of the original carries (+0x230.w = 1/32, Reference1499).
        alignas(16) uint8_t words[0x370]{};
        shadow::SceneWords(record,state,sizes.width?sizes.width:shadow::ClassSize(resolution),32,shadow::Constants{},words);
        std::memcpy(produced.scene_words.data(),words+0x1C0,0x1B0);
        std::memcpy(produced.sun.data(),sun,16);produced.on=shadows_on!=0;scene.shadow_frame=produced;}
    // A row on the first calls, every 600th, and at each change of the keys, the record's keys or the sun's source (the witness of
    // the shadow source, I-0140 tranche 7: in play and through a cutscene's fades).
    // Per context, at most 4,000 change rows a process (the final review's L4).
    static std::mutex last_lock;static std::map<uintptr_t,std::array<uint32_t,11>> last;static int change_rows{};
    const std::array<uint32_t,11> now{keys[0],keys[1],keys[2],keys[3],tags[0],tags[1],tags[2],tags[3],static_cast<uint32_t>(source),fixed_record,recorded?1u:0u};
    bool changed{};{const std::lock_guard hold(last_lock);if(change_rows<4000){auto& seen=last[context];changed=seen!=now;if(changed){seen=now;++change_rows;}}}
    if(!(n<=2 || n%600==0 || changed))return;
    uint32_t colour{};std::memcpy(&colour,record+0x8C,4);float angle[2]{};std::memcpy(angle,record,8);
    static const char* const sources[]{"missing","angle","light_set","blend","cut_light_set","cut_blend"};
    char text[2000]{};sprintf_s(text,"{\"event\":\"native_shadow_state\",\"call\":%lld,\"quality\":%s,\"shadows_on\":%u,\"lighting\":%u,"
        "\"row\":[%u,%u,%u,%u,%d],\"keys\":[%u,%u,%u,%u,%g],\"fixed_record\":%u,\"record\":%s,\"record_keys\":[%u,%u,%u,%u,%g],\"angle\":[%g,%g],"
        "\"shadow_colour\":%u,\"filter_mode\":%d,\"resolution_class\":%d,\"atlas\":%u,\"blur_passes\":%u,"
        "\"near\":%g,\"splits\":[%g,%g,%g],\"fade\":[%g,%g],\"local_mode\":%u,\"pipeline_gate\":[%d,%u,%u],\"sun_source\":\"%s\",\"sun\":[%g,%g,%g]}\n",
        n,quality?"true":"false",shadows_on,lighting,row.num,row.den,row.min,row.max,row.blur_add,keys[0],keys[1],keys[2],keys[3],weight,fixed_record,
        recorded?"true":"false",tags[0],tags[1],tags[2],tags[3],record_weight,angle[0],angle[1],colour,filter,resolution,
        sizes.width,sizes.blur_passes,state.near_z,state.split0,state.split1,state.split2,state.fade_start,state.fade_dist,local_mode,
        gate_words[0],gate_bytes[0],gate_bytes[1],sources[static_cast<size_t>(source)],sun[0],sun[1],sun[2]);
    Log(text);
}
// I-0140 tranche 31 (D-0303, MENUS-MAP 48): a Player menu view's shadow state for its traced self-shadow. 88AF0 keys the view's
// ShadowParam to {D27770(1), id} for both records (A = B: the weight's branch is the record itself), so 88A10's blend (DAF40) is
// the bank record (Banks::Shadow, 18C590's key rule) with DAF40's Angle direction and the colour +0x8C kept from the context's
// record (ShadowDefaults' 0xFF000000, the original's first, never written otherwise: MENUS-MAP 48's Shadow Color); the sun from
// ShadowSunDirection (every menu id has Shadow Light Id -1: the record's Angle); the scene words for FC_ShadowColor (+0x1D0), which
// the shadowed sun pass reads. The Shadow Quality row's shadows-on byte gates it as F90F0 gates the cascades. False: no shadow.
// I-0140 tranche 35 (the penumbra): the original's blur of the view's mask, 1D7850's gate and block: its passes the record's Blur
// Count (+0x84) plus the quality row's (1B5720, ShadowSizes; the menu ids 10 to 15: 4, so 6 at MAX), its scale
// [[GXRS+0x5368]+0x128] (1 without the object) x the record's Blur Radius (+0x88; 4, id 11: 8); none unless both are positive.
// The passes at most kMenuShadowBlurPassesMax (the view's plan key holds four bits).
bool CollectNativeMenuShadowState(NativeSelectedScene& scene,const std::array<uint32_t,2>& key,ds3light::ShadowSunSource& source) {
    source=ds3light::ShadowSunSource::Missing;
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    if(!banks)return false;
    float eps{},blend_eps{};Read(image+0x3D7ED70,&eps,4);Read(image+0x3D7ECE8,&blend_eps,4);
    alignas(16) uint8_t bank[0x94]{},record[0x94]{};
    if(!banks->Shadow(key[0],key[1],bank,sizeof bank))ds3light::ShadowDefaults(bank);
    ds3light::ShadowDefaults(record);ds3light::BlendShadowRecord(record,bank,bank,0.f,blend_eps);
    const bool quality=scene.options && scene.options->object_valid && scene.options->object.size()>=0x48;
    shadow::QualityRow row{};BYTE shadows_on=1;
    if(quality){const auto* q=scene.options->object.data();shadows_on=q[0x28];
        std::memcpy(&row.num,q+0x2C,4);std::memcpy(&row.den,q+0x30,4);std::memcpy(&row.min,q+0x34,4);std::memcpy(&row.max,q+0x38,4);
        std::memcpy(&row.blur_add,q+0x3C,4);if(!row.den)row.den=1;}
    const auto sizes=shadow::ShadowSizes(record,quality?&row:nullptr,false,2);const auto state=shadow::ShadowClamps(record,eps);
    int32_t resolution{};std::memcpy(&resolution,record+0x90,4);
    const uint32_t keys[4]{key[0],key[1],key[0],key[1]};float sun[4]{};
    source=ds3light::ShadowSunDirection(*banks,record,keys,sun);
    if(source==ds3light::ShadowSunSource::Missing || !shadows_on)return false;
    alignas(16) uint8_t words[0x370]{};
    shadow::SceneWords(record,state,sizes.width?sizes.width:shadow::ClassSize(resolution),32,shadow::Constants{},words);
    NativeSelectedScene::Shadow produced;std::memcpy(produced.scene_words.data(),words+0x1C0,0x1B0);std::memcpy(produced.sun.data(),sun,16);produced.on=true;
    {float factor=1.0f,radius{};uintptr_t root{},settings{};std::memcpy(&radius,record+0x88,4);
        if(Read(image+0x4961B50,&root,8) && root && Read(root+0x5368,&settings,8) && settings)Read(settings+0x128,&factor,4);
        const float blur_scale=factor*radius;
        if(0.0f<blur_scale && std::isfinite(blur_scale) && sizes.blur_passes){produced.blur_passes=std::min(sizes.blur_passes,kMenuShadowBlurPassesMax);produced.blur_scale=blur_scale;}}
    scene.shadow_frame=produced;return true;
}
// W5d GL-3 (GLOW-WAVE1-READY 5.1): the glow inputs of the main view. FB890 (lines 657-683) blends the Glow Filter records of
// the view context's index-10 keys (A +0x4028, B +0x4090, weight +0x40D0, epsilon [143D7ECE8]) through FUN_1402227C0; 1BB810
// runs mode B when Enable and New Glow Enable are set and the factor [[GXRS+0x5368]+0x80] (1 without the object) is
// positive. A record with Enable and no New Glow (mode A) is not in wave 1 (the row says so). no-glow.flag disables it.
void RecordNativeEffects(NativeSelectedScene& scene,uintptr_t view);   // FFX draw N5a, below
void CollectNativeGlowInputs(NativeSelectedScene& scene,uintptr_t view) {
    static const bool disabled=ModuleFlag(L"no-glow.flag");
    uintptr_t context{},root{},settings{};
    {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
        const auto found=scene_view_cpu_owners.find(view);if(found!=scene_view_cpu_owners.end())context=found->second.context;}
    std::shared_ptr<const ds3light::Banks> banks;{const std::lock_guard hold(gparam_lock);banks=gparam_banks;}
    uint32_t keys[4]{};float weight{},epsilon{},factor{1.0f};std::string sources;
    const bool read=context && banks && Read(context+0x4028,&keys[0],8) && Read(context+0x4090,&keys[2],8) && Read(context+0x40D0,&weight,4) &&
        Read(image+0x3D7ECE8,&epsilon,4);
    if(Read(image+0x4961B50,&root,8) && root && Read(root+0x5368,&settings,8) && settings)Read(settings+0x80,&factor,4);
    ds3light::Banks::GlowRecord record;if(read)record=banks->Glow(keys[0],keys[1],keys[2],keys[3],weight,epsilon,&sources);
    const bool mode_b=read && record.enable && record.new_glow,mode_a=read && record.enable && !record.new_glow;
    scene.glow.reset();
    const auto programs=mode_b && 0.0f<factor && !disabled?AcquireNativeGlowPrograms():nullptr;
    if(programs){NativeGlowInput input;input.blur_scale=record.blur_scale;input.programs=programs;
        uint8_t image_rgb[16]{},image_final[16]{};uint32_t alpha_bits{};std::memcpy(&alpha_bits,&record.alpha,4);
        ds3native::glow::ParamImage(record.rgb,alpha_bits,factor,image_rgb);ds3native::glow::ParamImage(record.final_scale,alpha_bits,factor,image_final);
        std::memcpy(&input.rgb_scale,image_rgb,4);std::memcpy(&input.final_scale,image_final,4);input.alpha_bits=alpha_bits;scene.glow=input;}
    static LONG64 calls{};static std::array<uint32_t,8> last{};const auto n=InterlockedIncrement64(&calls);
    std::array<uint32_t,8> now{keys[0],keys[1],keys[2],keys[3],record.enable,record.new_glow,scene.glow?1u:0u,0};std::memcpy(&now[7],&weight,4);
    if(n<=3 || n%600==0 || now!=last){last=now;char row[512]{};
        sprintf_s(row,"{\"event\":\"native_glow_inputs\",\"call\":%lld,\"read\":%s,\"keys\":[%u,%u,%u,%u],\"weight\":%g,\"enable\":%u,\"new_glow\":%u,"
            "\"blur_scale\":%g,\"rgb\":%g,\"alpha\":%g,\"final\":%g,\"factor\":%g,\"sources\":\"%.40s\",\"mode_a\":%s,\"disabled\":%s,\"glow\":%s}\n",
            static_cast<long long>(n),read?"true":"false",keys[0],keys[1],keys[2],keys[3],weight,record.enable,record.new_glow,record.blur_scale,record.rgb,
            record.alpha,record.final_scale,factor,sources.c_str(),mode_a?"true":"false",disabled?"true":"false",scene.glow?"true":"false");Log(row);}
}
}  // namespace
