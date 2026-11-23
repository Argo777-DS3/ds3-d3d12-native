// Engine-step services: Load Game portraits, drawable registration, probe lighting, cloth and dynamic mesh streams, input
// latching, the render step and EndFrame. Part of game_bootstrap.cpp's single translation unit (the order is there): it
// relies on every earlier part (16_offscreen_and_movies.inc before it).
#pragma once
#include "16_offscreen_and_movies.inc"
namespace {
// 1159: ADEF50 (Load Game summary, MenuSaveDataSummary -> AD7DD0 -> ADAAA0) hands each valid save record's
// appearance to its portrait 14478D680[slot] with no null check (Native1116/1158: 57F8A3 with every slot nil,
// I-0067 MENUS-MAP sections 1, 7). Transcription with the one native difference: a slot without its portrait
// (formerly the explicit omission of AE5B70; since 1170 AE5B70 builds them genuinely) is skipped.
void __fastcall NativeLoadGameSummaryPortraits() noexcept {
    std::array<uintptr_t,10> profiles{};
    if(!Read(image+0x478D680,profiles.data(),sizeof(profiles)))Refuse("native_menu_profiles_registry_unreadable");
    const auto list=reinterpret_cast<uintptr_t(__fastcall*)()>(image+0x597400)();
    uint32_t skipped{};
    for(uint32_t slot=0;slot<10;++slot){
        const auto record=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uint32_t)>(image+0x5ACA10)(list,slot);
        if(!record || !reinterpret_cast<bool(__fastcall*)(uint32_t)>(image+0xC318B0)(slot))continue;
        const auto profile=profiles[slot];if(!profile){++skipped;continue;}
        BYTE appearance_flag{},state{};Read(record+0x240,&appearance_flag,1);Read(record+0x246,&state,1);
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xD6B9F0)(profile,record+0x150);
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0xD675A0)(profile,record+0x4C);
        reinterpret_cast<void(__fastcall*)(uintptr_t,BYTE)>(image+0xD67670)(profile,appearance_flag);
        reinterpret_cast<void(__fastcall*)(uintptr_t,BYTE)>(image+0xD67660)(profile,1);
        reinterpret_cast<void(__fastcall*)(uintptr_t,BYTE)>(image+0xD67680)(profile,static_cast<BYTE>(state&1));
        reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t)>(image+0xD668A0)(profile,slot*2);
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xD67540)(profile);
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0xD67560)(profile);
    }
    static LONG64 reported{};if(InterlockedIncrement64(&reported)<=4){char row[200]{};
        sprintf_s(row,"{\"event\":\"native_load_game_summary\",\"portraits_skipped\":%u,\"visual_omission\":\"menu_profile_portraits\"}\n",skipped);Log(row);}
}
void __fastcall NativeDrawableRegistration(uintptr_t drawable,uintptr_t owner) noexcept {
    // 2D860 (inventory910): registers a model drawable (+0x40 description) into
    // the scene drawable owner from render service4796298 (+0x10 -> +0x58),
    // through C9E60 (owner heap +0x80, list +0x118). In native execution that
    // owner is absent. Bounded diagnostic: record the service scene layout and
    // leave this drawable unregistered (the genuine result of a failed C9E60,
    // drawable+0x68 stays null). This is not the native registration contract.
    if(owner){drawable_registration_original(drawable,owner);return;}
    static LONG64 samples{};if(InterlockedIncrement64(&samples)<=4){
        uintptr_t service{},scene{},primary{},secondary{},scene_table{},primary_table{};
        Read(image+0x4796298,&service,8);if(service)Read(service+0x10,&scene,8);
        if(scene){Read(scene,&scene_table,8);Read(scene+0x50,&primary,8);Read(scene+0x58,&secondary,8);}
        if(primary)Read(primary,&primary_table,8);
        bool native_view{},native_context{};
        {const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
            native_view=scene_view_cpu_owners.count(primary)!=0;native_context=scene_context_cpu_owners.count(primary)!=0;}
        const auto rva=[](uintptr_t value){return static_cast<unsigned long long>(value>=image?value-image:value);};
        char row[512]{};sprintf_s(row,"{\"event\":\"native_drawable_owner_unavailable\",\"caller_rva\":%llu,\"scene_table_rva\":%llu,\"primary\":%llu,\"primary_table_rva\":%llu,\"primary_native_view\":%s,\"primary_native_context\":%s,\"secondary\":%llu,\"diagnostic_only\":true}\n",
            rva(reinterpret_cast<uintptr_t>(_ReturnAddress())),rva(scene_table),static_cast<unsigned long long>(primary),rva(primary_table),
            native_view?"true":"false",native_context?"true":"false",static_cast<unsigned long long>(secondary));Log(row);}
}
// Optimisation review (Thomas's m33 check, the per-instance SH; TONEMAP-SKIN §10): the anim block's SH rows (q+0x130..+0x15F,
// strength q+0x16C, owned flag q+0x170) that 5B3D0 writes as cbInstanceData +0x80..+0xAF are computed by 61950 (a new probe
// record, instant) and 1AC840 (the per-frame blend), both only while the old renderer's device [GXRS+0x4DD0] exists. Natively
// that device never exists (D-0096), so the rows stayed zero and q+0x170 unset: the characters and objects lost their probe
// ambient and the map pieces' rows came from elsewhere. The native owner runs the same CPU body on the block p = q+0xF0:
// the probe record's 13 shorts with the engine's SH basis constants (.rdata 143D7F940, strength scale 143D7ED0C), or, without
// a record, the light-set rows of the block's key (p+0xB0) from the native banks (1495F0 of the set: the engine's own bank
// holds no file natively) when p+0x71 asks for them, zero otherwise; then the blend by factor f. Values are read from the
// image, never assumed.
void NativeShBlockBody(uintptr_t p,float f);
void NativeShBlock(uintptr_t p,float f) noexcept {try{NativeShBlockBody(p,f);}catch(...){}}   // pre-merge review: the banks may throw inside the engine's noexcept hooks
void NativeShBlockBody(uintptr_t p,float f) {
    float basis[4]{},strength_scale{},one{1.f};
    if(!Read(image+0x3D7F940,basis,16) || !Read(image+0x3D7ED0C,&strength_scale,4) || !Read(image+0x3D7EE10,&one,4))return;
    uintptr_t record{};uint8_t lightset{},initialised{};if(!Read(p+0x88,&record,8) || !Read(p+0x71,&lightset,1) || !Read(p+0x70,&initialised,1))return;
    const auto set_rows=[&](std::array<float,12>& rows){uint32_t key[2]{};rows={};return Read(p+0xB0,key,8) && NativeInstanceLightRows(key,rows);};
    std::array<float,12> target{};float strength_target=one;
    if(!record){if(!lightset){target={};*reinterpret_cast<uint8_t*>(p+0x80)=1;}
        else{set_rows(target);*reinterpret_cast<uint8_t*>(p+0x80)=0;}
        *reinterpret_cast<uint8_t*>(p+0x81)=1;}
    else{int16_t s[13]{};if(!Read(p+0x90,s,26))return;
        for(int r=0;r<3;++r){target[4*r]=float(int(s[9+r]))*basis[0];target[4*r+1]=float(int(s[3+r]))*basis[1];target[4*r+2]=float(int(s[6+r]))*basis[2];target[4*r+3]=float(int(s[r]))*basis[3];}
        *reinterpret_cast<uint16_t*>(p+0x80)=1;strength_target=float(int(s[12]))*strength_scale;}
    auto* rows=reinterpret_cast<float*>(p+0x40);
    if(!initialised){*reinterpret_cast<uint8_t*>(p+0x70)=1;std::array<float,12> first{};if(lightset)set_rows(first);std::memcpy(rows,first.data(),48);}
    for(int k=0;k<12;++k)rows[k]=f*(target[k]-rows[k])+rows[k];
    auto& strength=*reinterpret_cast<float*>(p+0x7C);strength=(strength_target-strength)*f+strength;
}
// I-0088 final review M1: E07's settings shadow, named there only while the effect record executes (on the collect's side worker
// since I-0088), counts as absent.
bool NativeDeviceAbsent() noexcept {uintptr_t root{},device{};return Read(image+0x4961B50,&root,8) && root && Read(root+0x4DD0,&device,8) &&
    (!device || device==effect_draw::DeviceShadow());}
using ShRecordSet=void(__fastcall*)(uintptr_t,uintptr_t,char);
ShRecordSet sh_record_set_original{};
void __fastcall NativeShRecordSet(uintptr_t q,uintptr_t record,char smooth) noexcept {
    // 61950's own early outs: the reset byte +0x1A8 forces an instant change; the same record does nothing; a smooth change
    // only restarts the clock (1AC840 blends). An instant change sets the rows at once (its clock factor is 1).
    uint8_t reset{};uintptr_t current{};Read(q+0x1A8,&reset,1);Read(q+0x178,&current,8);
    const bool instant=(reset || !smooth) && (current?q+0x180:0)!=record;
    // I-0120 D3: 61950 re-reads [[GXRS]+0x4DD0] and with it set samples through root+0x41E0; it waits while the effect record
    // names the settings shadow there (a rare call: a light set record change).
    if(effect_draw::WaitEngineShUse()){static LONG64 timeouts{};if(const auto n=InterlockedIncrement64(&timeouts);!(n&(n-1))){char row[96]{};
        sprintf_s(row,"{\"event\":\"native_sh_record_set_wait_timeout\",\"count\":%lld}\n",n);Log(row);}}
    struct Leave {~Leave(){effect_draw::LeaveEngineShUse();}} leave;
    sh_record_set_original(q,record,smooth);
    if(instant && NativeDeviceAbsent())NativeShBlock(q+0xF0,1.f);
    if(static const bool sh_witness=ModuleFlag(L"light-observe.flag");sh_witness){static LONG64 calls{},with_record{};const auto n=InterlockedIncrement64(&calls);if(record)InterlockedIncrement64(&with_record);
        if(n<=8 || n%2000==0){char row[200]{};sprintf_s(row,"{\"event\":\"native_sh_record_set\",\"calls\":%lld,\"with_record\":%lld,\"record\":%s,\"smooth\":%d,\"instant\":%s}\n",
            n,with_record,record?"true":"false",int(smooth),instant?"true":"false");Log(row);}}
}
void __fastcall NativeObjectProbeLightBlend(uintptr_t object,float delta) noexcept {
    // 1AC840 (inventory903/904): per-object light-probe blend for characters.
    // It advances the clamped blend clock +74 (limit +78), then, only when the
    // old renderer root4961B50 has its device +4DD0, samples probe
    // coefficients through root+41E0 and blends +40..+7C. Native execution has
    // no such root: keep the engine's own device-absent behaviour (clock only).
    // Probe lighting of characters is an explicit initial omission until the
    // native light-probe owner exists; no root, device or coefficient is made up.
    uintptr_t root{};
    if(!Read(image+0x4961B50,&root,8))Refuse("native_probe_light_root_unreadable");
    // I-0120 D3: while the effect record names the settings shadow at root+0x4DD0, 1AC840 would sample through root+0x41E0; its
    // device-absent path (the clock alone) is then taken natively below, followed by the native SH block as after the original.
    const bool shadow_named=root && !effect_draw::EnterEngineShUse();
    if(root && !shadow_named){struct Leave {~Leave(){effect_draw::LeaveEngineShUse();}} leave;object_probe_light_blend_original(object,delta);
        // The native SH owner (above): 1AC840's factor from the clock it just advanced: delta over what remains to the limit,
        // at most 1, 1 when nothing remains or the limit is not positive.
        if(NativeDeviceAbsent()){float clock{},limit{},epsilon{},one{1.f};
            if(Read(object+0x74,&clock,4) && Read(object+0x78,&limit,4) && Read(image+0x3D7ECE8,&epsilon,4) && Read(image+0x3D7EE10,&one,4)){
                float f=one;if(limit>0.f){const float remaining=std::max(limit-clock,0.f);if(remaining>=epsilon)f=std::min(delta/remaining,1.f);}
                NativeShBlock(object,f);}}
        return;}
    float clock{},limit{};
    if(!Read(object+0x74,&clock,4) || !Read(object+0x78,&limit,4)){NoteNativeSkip("probe_light_object_unreadable",object);return;}   // I-0101
    clock+=delta;if(clock<0.0f)clock=0.0f;else if(clock>limit)clock=limit;
    *reinterpret_cast<float*>(object+0x74)=clock;
    if(shadow_named){float epsilon{},one{1.f};   // D3: the factor as above, from the clock just advanced
        if(Read(image+0x3D7ECE8,&epsilon,4) && Read(image+0x3D7EE10,&one,4)){
            float f=one;if(limit>0.f){const float remaining=std::max(limit-clock,0.f);if(remaining>=epsilon)f=std::min(delta/remaining,1.f);}
            NativeShBlock(object,f);}
        return;}
    static LONG64 samples{};if(InterlockedIncrement64(&samples)==1)
        Log("{\"event\":\"native_object_probe_lighting_unavailable\",\"engine_device_absent_branch\":true,\"legacy_root_created\":false}\n");
}
uint64_t cloth_stream_generation{};
bool cloth_streams_closed{};
using ClothVertexUpload=uint32_t*(__fastcall*)(uintptr_t,uint32_t*);
using ClothSetRelease=void(__fastcall*)(uintptr_t);
ClothVertexUpload cloth_vertex_upload_original{};ClothSetRelease cloth_set_release_original{};
uint32_t* __fastcall NativeClothVertexUpload(uintptr_t drawable,uint32_t* status) noexcept {
    census::Note(census::ClothUpload);   // R13 cR0
    // 23D5BC0 (drawable vt+0x48, per simulated frame): step 1 tests the descriptor and its buffer object, step 2
    // replicates the simulated values onto duplicated vertices inside the CPU copy (34F1B0 u32 pairs, 34F090 u64
    // pairs). Steps 3-6 (pool context, Map discard, memcpy, Unmap) become one native upload of the whole copy.
    try {
        uintptr_t display{};Read(image+0x4961AB0,&display,8);
        if(display)return cloth_vertex_upload_original(drawable,status);
        uintptr_t desc{},object{},owner{},pairs_owner{},copy{};uint32_t mesh{},bytes{};
        if(!Read(drawable+0x120,&desc,8) || !desc || !Read(desc+0x40,&object,8) || !object){*status=1;return status;}
        if(!Read(desc,&mesh,4) || !Read(desc+8,&owner,8) || !owner || !Read(owner+0x28,&pairs_owner,8) || !pairs_owner ||
           !Read(desc+0x48,&copy,8) || !copy || !Read(desc+0x58,&bytes,4) || !bytes || bytes>(64u<<20)){*status=1;return status;}
        uintptr_t pairs{};uint32_t count{};BYTE wide{};
        if(Read(pairs_owner+0x18,&pairs,8) && pairs && Read(pairs+16ull*mesh+4,&count,4) && Read(pairs_owner+0xC,&wide,1)){
            uintptr_t data{};Read(pairs+16ull*mesh+8,&data,8);
            reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uint32_t)>(image+(wide?0x34F090:0x34F1B0))(desc,data,count);}
        // I-0120 A6: a stream already simulated stays so (sticky), so its upload needs no fingerprint; read under the lock that
        // also refuses late uploads after the close.
        bool known_simulated{};
        {const std::lock_guard hold(cloth_stream_lock);if(cloth_streams_closed || !gpu.device){*status=1;return status;}
            if(const auto previous=cloth_streams.find(object);previous!=cloth_streams.end())known_simulated=previous->second->simulated;}
        const auto* source=reinterpret_cast<const BYTE*>(copy);std::vector<BYTE> current(source,source+bytes);   // no zero-fill first
        bool simulated{};auto stream=std::make_shared<NativeClothStream>();stream->bytes=bytes;
        const auto fingerprint=[&]{stream->fingerprint=1469598103934665603ull;for(const auto b:current)stream->fingerprint=(stream->fingerprint^b)*1099511628211ull;};
        if(!known_simulated)fingerprint();
        // B6: no frame lock (resource creation is free-threaded and no queue work is involved); the cloth lock keeps the
        // device alive against CloseNativeClothStreams, which precedes the renderer's close.
        {const std::lock_guard hold(cloth_stream_lock);if(cloth_streams_closed || !gpu.device){*status=1;return status;}
            stream->resource=PublishNativeStreamBytes(current,stream->version);gpu_audit.dynamic_stream_updates.fetch_add(1,std::memory_order_relaxed);
            gpu_audit.dynamic_stream_bytes.fetch_add(current.size(),std::memory_order_relaxed);}
        {const std::lock_guard hold(cloth_stream_lock);if(cloth_streams_closed){*status=1;return status;}
            stream->frame=native_cloth_frames.load(std::memory_order_relaxed);   // I-0134: stamped where it is paired (its second final review's L4)
            // Only a copy the simulation has rewritten (content differing from the first upload) replaces the authored stream.
            if(const auto previous=cloth_streams.find(object);previous!=cloth_streams.end()){stream->first=previous->second->first;
                if(known_simulated && !previous->second->simulated)fingerprint();   // the entry replaced between the two locks
                stream->simulated=previous->second->simulated || stream->fingerprint!=stream->first;
                // I-0134: the replaced copy, the same bytes long and itself simulated (the authored first upload would make the first
                // simulated frame's motion the jump from the authored pose; its step review's L1), for the Velo rows' previous position:
                // the last frame's upload (its final review's M1: a copy older by several frames would give their whole motion as one
                // frame's). A second upload in one frame (two simulation steps) keeps the replaced copy's own previous one, the last
                // frame's, so the row carries the frame's whole motion, not its last step's (the Fable 5.2 read, 2026-11-12).
                const auto& replaced=*previous->second;
                if(replaced.bytes==bytes && replaced.simulated){
                    if(stream->frame-replaced.frame==1){stream->previous_resource=replaced.resource;stream->previous_version=replaced.version;}
                    else if(stream->frame==replaced.frame){stream->previous_resource=replaced.previous_resource;stream->previous_version=replaced.previous_version;}}}
            else{if(known_simulated)fingerprint();   // released between the two locks: this upload is a new first one
                stream->first=stream->fingerprint;}
            simulated=stream->simulated;stream->generation=++cloth_stream_generation;cloth_streams[object]=std::move(stream);}
        static LONG64 uploads{};if(const auto n=InterlockedIncrement64(&uploads);n<=4 || n%200==0){char row[260]{};
            sprintf_s(row,"{\"event\":\"native_cloth_vertex_upload\",\"count\":%lld,\"bytes\":%u,\"pairs\":%u,\"simulated\":%s,\"vertex0\":[%g,%g,%g],\"legacy_context\":false}\n",n,bytes,count,simulated?"true":"false",
                reinterpret_cast<const float*>(current.data())[0],reinterpret_cast<const float*>(current.data())[1],reinterpret_cast<const float*>(current.data())[2]);Log(row);}
        *status=0;return status;
    }catch(...){Refuse("native_cloth_vertex_upload_failed");}
    *status=1;return status;
}
void __fastcall NativeClothSetRelease(uintptr_t set) noexcept {
    census::NoteRetirement(census::ClothRelease,_ReturnAddress());   // R13 cR0
    // 23B7280: the engine frees the copies and drops its references to the buffer objects; their native
    // versions are forgotten first (selected draws keep their own references).
    try {
        uintptr_t begin{},end{};
        if(Read(set+0x18,&begin,8) && Read(set+0x20,&end,8) && begin && end>=begin && (end-begin)/8<=4096){
            const std::lock_guard hold(cloth_stream_lock);
            for(uintptr_t at=begin;at<end;at+=8){uintptr_t object{};if(Read(at,&object,8) && object)cloth_streams.erase(object);}}
    }catch(...){Refuse("native_cloth_set_release_failed");}
    cloth_set_release_original(set);
}
void CloseNativeClothStreams() {const std::lock_guard hold(cloth_stream_lock);cloth_streams_closed=true;cloth_streams.clear();}
uint8_t __fastcall NativeDynamicMeshBufferSet(uintptr_t set,uintptr_t model,uintptr_t source,uintptr_t owner) noexcept {
    // 23B7960 (export728/901) builds, for each admitted mesh of a loading
    // model, an old-API vertex buffer from the old display object4961AB0 plus a
    // CPU copy, recorded in the set's buffer array+18 and copy list+38. Its only
    // consumer resolution 23B6DB0/23B74A0 yields an empty descriptor for a mesh
    // without an entry, which 23D6010 already skips. Native execution keeps the
    // engine's own empty-set shape: no old GPU object is created or invented,
    // and the family is an explicit initial omission until its native dynamic
    // stream owner (creation, CPU update, selected use, retirement) exists.
    uintptr_t display{};
    if(!Read(image+0x4961AB0,&display,8))Refuse("native_dynamic_mesh_display_unreadable");
    if(display)return dynamic_mesh_buffer_set_original(set,model,source,owner);
    if(!set || !model || !source || !owner)return 0;
    uintptr_t begin{},end{},copies{},copies_end{},allocator{};int32_t meshes{};
    if(!Read(set+0x18,&begin,8) || !Read(set+0x20,&end,8) || !Read(set+0x38,&copies,8) || !Read(set+0x40,&copies_end,8) ||
       !Read(set,&allocator,8) || !Read(model+0xA0,&meshes,4) || begin!=end || copies!=copies_end || !allocator || meshes<0 || meshes>4096)
        Refuse("native_dynamic_mesh_set_not_pristine");
    *reinterpret_cast<uint32_t*>(set+0x58)=0;
    // 1184 (MENUS-MAP 65): 23B7960 transcribed with the engine's own CPU-only buffer form. The GX buffer object
    // (0x28 bytes, vtable 3A3A428: refcount, scalar destructor, allocator, alt description) is made by the genuine
    // 192B300 (misc 0x4000: no D3D11 object, alt block from the allocator) and 1932410; the CPU copies start as the
    // authored stream-0 bytes, allocated from set[0], so Havok's writes and 34F1B0 act on them as in the original.
    // The upload (23D5BC0 steps 3-6) is native (NativeClothVertexUpload); release stays the engine's (23B7280).
    try {
        {uintptr_t iterator{},fill{};reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t*,uintptr_t,uint64_t,uintptr_t*)>(image+0x34EBA0)
            (set+0x18,&iterator,end,static_cast<uint64_t>(meshes),&fill);}
        struct Entry {int32_t mesh{};uint32_t bytes{};uintptr_t copy{};};static_assert(sizeof(Entry)==16);
        std::vector<Entry> entries;uintptr_t mesh_table{},material_table{},material_table_end{},names{},headers{};
        if(!Read(model+0xA8,&mesh_table,8) || !Read(model+0x1F8,&material_table,8) || !Read(model+0x200,&material_table_end,8) ||
           !Read(model+0x20,&names,8) || !Read(model+0x158,&headers,8) || !Read(set+0x18,&begin,8))throw std::runtime_error("cloth set model");
        std::array<uint32_t,4> rejected{};   // witness: material +0x22, hcl name (23C5070), POSITION buffer (23B6C50), authored bytes
        for(int32_t mesh=0;mesh<meshes;++mesh){
            const auto entry=mesh_table+0x48ull*mesh;uint16_t material_index{},name_index{};uintptr_t material{},name{};BYTE cloth{};
            if(!Read(entry,&material_index,2) || !Read(entry+0x16,&name_index,2))throw std::runtime_error("cloth set mesh");
            if(material_index>=(material_table_end-material_table)/8 || !Read(material_table+8ull*material_index,&material,8) || !material ||
               !Read(material+0x22,&cloth,1) || !cloth){++rejected[0];continue;}
            if(!Read(names+0x30ull*name_index+0x20,&name,8) ||
               !reinterpret_cast<char(__fastcall*)(uintptr_t,uintptr_t)>(image+0x23C5070)(source,name)){++rejected[1];continue;}
            const auto vertex_buffer=reinterpret_cast<int32_t(__fastcall*)(int32_t,uintptr_t,uintptr_t,uintptr_t)>(image+0x23B6C50)
                (mesh,model+0x98,model+0x148,model+0x170);
            if(vertex_buffer<0){++rejected[2];continue;}
            const auto header=headers+0x90ull*vertex_buffer;uintptr_t authored{};uint32_t count{},stride{};
            if(!Read(header+0x80,&authored,8) || !Read(header+0x7C,&count,4) || !Read(header+0x68,&stride,4))throw std::runtime_error("cloth set vertex header");
            if(!authored){++rejected[3];continue;}
            const uint32_t bytes=static_cast<uint32_t>((uint64_t(count)*stride+3)&~uint64_t(3));
            if(!bytes || bytes>(64u<<20))throw std::runtime_error("cloth set extent");
            uint32_t description[7]{bytes,2,1,1,0x4000,0,0};uintptr_t handle[2]{};
            reinterpret_cast<void(__fastcall*)(uintptr_t*,uintptr_t,uint32_t*,uintptr_t,uintptr_t)>(image+0x192B300)(handle,0,description,0,allocator);
            if(!handle[1])throw std::runtime_error("cloth set alt buffer");
            const auto object=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0x28,8,allocator);
            if(!object)throw std::bad_alloc();
            reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t*,uintptr_t)>(image+0x1932410)(object,handle,allocator);
            *reinterpret_cast<uintptr_t*>(begin+8ull*mesh)=object;
            uintptr_t allocator_table{};if(!Read(allocator,&allocator_table,8))throw std::runtime_error("cloth set allocator");
            const auto copy=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,size_t)>(*reinterpret_cast<uintptr_t*>(allocator_table+0x48))(allocator,bytes);
            if(!copy)throw std::bad_alloc();
            std::memcpy(reinterpret_cast<void*>(copy),reinterpret_cast<const void*>(authored),bytes);
            entries.push_back({mesh,bytes,copy});*reinterpret_cast<uint32_t*>(set+0x58)+=bytes;
        }
        uintptr_t copies_begin{};Read(set+0x38,&copies_begin,8);
        if(!entries.empty())reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,const Entry*,const Entry*)>(image+0x34F7E0)
            (set+0x38,copies_begin,entries.data(),entries.data()+entries.size());
        *reinterpret_cast<uintptr_t*>(set+0x08)=model;*reinterpret_cast<uintptr_t*>(set+0x10)=owner;
        static LONG64 samples{};if(InterlockedIncrement64(&samples)<=24){char row[400]{};
            uintptr_t vector_begin{},vector_end{};Read(set+0x18,&vector_begin,8);Read(set+0x20,&vector_end,8);
            sprintf_s(row,"{\"event\":\"native_cloth_set_created\",\"set\":%llu,\"model\":%llu,\"meshes\":%d,\"vector\":%llu,\"cloth_meshes\":%zu,\"bytes\":%u,\"rejected\":[%u,%u,%u,%u],\"legacy_gpu_created\":false}\n",
                static_cast<unsigned long long>(set),static_cast<unsigned long long>(model),meshes,static_cast<unsigned long long>((vector_end-vector_begin)/8),
                entries.size(),*reinterpret_cast<uint32_t*>(set+0x58),rejected[0],rejected[1],rejected[2],rejected[3]);Log(row);}
    }catch(const std::exception& error){char row[256]{};sprintf_s(row,"{\"event\":\"native_cloth_set_failure\",\"message\":\"%.120s\"}\n",error.what());Log(row);
        Refuse("native_cloth_set_failed");}
    return 1;
}
// I-0067 NATIVE-INPUT-LATCH N1-N4: the game samples the mouse buttons once per update (23891E0, GetKeyboardState), and a
// native update can last 0.67-0.84 s (Load Game portraits), so a press that begins and ends between two updates is lost.
// The observer is a WH_GETMESSAGE hook on the game window's thread (only input addressed to the game window, removed from
// its queue): each button down increments that button's press count. 23891E0 runs once per update for each of the game's
// mouse objects (three: Native1229), each with its own state machine, so each object keeps the count it has seen: after
// the original update, a new press that the object's state machine does not hold (it began and ended between two updates)
// gets one forced press update (+8 edge, +0xA held, +9 clear); the original makes the release edge at the next update.
std::array<std::atomic<uint64_t>,3> native_mouse_presses{};
std::atomic<bool> native_mouse_down[3]{};
std::array<std::atomic<uint64_t>,256> native_key_presses{};
HHOOK native_input_hook{};
std::atomic<HWND> native_input_window{};   // the game window: only its input (and its children's) is counted (N1)
LRESULT CALLBACK NativeInputLatchHook(int code,WPARAM remove,LPARAM message) noexcept {
    if(code==HC_ACTION && remove==PM_REMOVE && message && reinterpret_cast<const MSG*>(message)->hwnd &&
       GetAncestor(reinterpret_cast<const MSG*>(message)->hwnd,GA_ROOT)==native_input_window.load()){const auto& m=*reinterpret_cast<const MSG*>(message);
        const int button=m.message==WM_LBUTTONDOWN || m.message==WM_LBUTTONUP || m.message==WM_LBUTTONDBLCLK?0:
            m.message==WM_RBUTTONDOWN || m.message==WM_RBUTTONUP || m.message==WM_RBUTTONDBLCLK?1:
            m.message==WM_MBUTTONDOWN || m.message==WM_MBUTTONUP || m.message==WM_MBUTTONDBLCLK?2:-1;
        if(button>=0){const bool down=m.message!=WM_LBUTTONUP && m.message!=WM_RBUTTONUP && m.message!=WM_MBUTTONUP;
            native_mouse_down[button]=down;if(down && !input_focus::PageBlocks())++native_mouse_presses[button];}   // R12h's L7 (the step review's L3): a press under the precompile page never counted, so never replayed after it
        // The mouse wheel over our Advanced page (02b, Thomas 2026-11-17): its delta counted for the page's update.
        if(m.message==WM_MOUSEWHEEL && !input_focus::PageBlocks())rekindled_wheel_delta.fetch_add(GET_WHEEL_DELTA_WPARAM(m.wParam));
        // NATIVE-INPUT-LATCH D4: key presses (not auto-repeats) by DirectInput scancode (extended keys +0x80; Pause and
        // NumLock do not follow that rule and come out swapped, Print Screen too: not bound by the game's menus).
        if((m.message==WM_KEYDOWN || m.message==WM_SYSKEYDOWN) && !(m.lParam&(1ll<<30))){
            const auto scancode=static_cast<uint32_t>((m.lParam>>16)&0xFF)|((m.lParam&(1ll<<24))?0x80u:0u);
            if(!input_focus::PageBlocks())++native_key_presses[scancode&0xFF];}}   // R12h's L7: as the buttons
    return CallNextHookEx(native_input_hook,code,remove,message);
}
// NATIVE-INPUT-LATCH §6 D1-D2: the decide action reads the DirectInput devices (mouse 14183C540: GetDeviceState
// DIMOUSESTATE2, buttons +0x1AC..; keyboard 14183BE70: 256 bytes at +0x1A0), polled once per update. After a successful
// original poll, a press counted since the object's last poll and absent from the buffer sets bit 7 of its byte for this
// one poll; the next poll releases it. A failed poll is never forced.
using DirectInputPoll=uint64_t(__fastcall*)(uintptr_t);
DirectInputPoll native_di_mouse_poll_original{},native_di_keyboard_poll_original{};
void NoteNativeDirectInputLatch(const char* device,uint32_t input) noexcept {
    static LONG64 forced[2]{};const auto n=InterlockedIncrement64(&forced[device[0]=='k']);   // 1253: bounded per device (keyboard rows after many mouse rows)
    if(n<=64 || n%100==0){char row[160]{};sprintf_s(row,"{\"event\":\"native_input_latch_di\",\"tick\":%llu,\"device\":\"%s\",\"input\":%u,\"poll_ok\":true,\"forced\":%lld}\n",
        static_cast<unsigned long long>(GetTickCount64()),device,input,n);Log(row);}
}
uint64_t __fastcall NativeDirectInputMousePoll(uintptr_t device) noexcept {
    const auto raw=native_di_mouse_poll_original(device);const bool result=static_cast<uint8_t>(raw)!=0;   // the poll returns its bool in AL
    const bool away=input_focus::Mouse(device,raw);   // I-0113 gap 26: the motion read while the game was away dropped (04)
    static std::unordered_map<uintptr_t,std::array<uint64_t,3>> seen;   // game thread (Device::Update)
    auto [entry,fresh]=seen.try_emplace(device);
    for(int button=0;button<3;++button){const auto presses=native_mouse_presses[button].load();
        if(fresh || presses==entry->second[button]){entry->second[button]=presses;continue;}
        entry->second[button]=presses;
        if(!result || away)continue;   // gap 26: a press counted while the game was away not forced
        // WM_*BUTTON* are logical buttons (after SwapMouseButton); rgbButtons are physical.
        const int physical=button<2 && GetSystemMetrics(SM_SWAPBUTTON)?1-button:button;
        const auto byte=reinterpret_cast<volatile uint8_t*>(device+0x1AC+physical);
        if(!(*byte&0x80)){*byte|=0x80;NoteNativeDirectInputLatch("mouse",static_cast<uint32_t>(button));}}
    return raw;
}
uint64_t __fastcall NativeDirectInputKeyboardPoll(uintptr_t device) noexcept {
    const auto raw=native_di_keyboard_poll_original(device);const bool result=static_cast<uint8_t>(raw)!=0;
    const bool away=input_focus::Keyboard(device,raw);   // I-0113 gap 26: no key while the game is not in front (04)
    static std::unordered_map<uintptr_t,std::array<uint64_t,256>> seen;
    auto [entry,fresh]=seen.try_emplace(device);
    for(uint32_t key=0;key<256;++key){const auto presses=native_key_presses[key].load();
        if(fresh || presses==entry->second[key]){entry->second[key]=presses;continue;}
        entry->second[key]=presses;
        if(!result || away)continue;   // gap 26: a press counted while the game was away not forced
        const auto byte=reinterpret_cast<volatile uint8_t*>(device+0x1A0+key);
        if(!(*byte&0x80)){*byte|=0x80;NoteNativeDirectInputLatch("keyboard",key);}}
    return raw;
}
bool native_input_retired{};   // the observer is removed once the native session stops
void ApplyNativeMouseLatch(uintptr_t mouse) noexcept {
    static bool attempted{};if(native_input_retired)return;
    HWND window{};
    if(!attempted && Read(mouse,&window,sizeof(window)) && window){attempted=true;
        HMODULE self{};const DWORD thread=GetWindowThreadProcessId(window,nullptr);
        if(thread && GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
               reinterpret_cast<LPCWSTR>(&NativeInputLatchHook),&self))
            {native_input_window=GetAncestor(window,GA_ROOT);native_input_hook=SetWindowsHookExW(WH_GETMESSAGE,&NativeInputLatchHook,self,thread);}
        char row[160]{};sprintf_s(row,"{\"event\":\"native_input_latch_observer\",\"installed\":%s,\"thread\":%lu}\n",native_input_hook?"true":"false",thread);Log(row);}
    if(!native_input_hook)return;
    if(control->stop){UnhookWindowsHookEx(native_input_hook);native_input_retired=true;return;}
    // Game thread only (23891E0's caller); a new object starts from the current counts (no replay of old presses).
    static std::unordered_map<uintptr_t,std::array<uint64_t,3>> seen;static LONG64 forced_updates{};
    auto [entry,fresh]=seen.try_emplace(mouse);
    const bool page=input_focus::PageBlocks();   // R12h's L7: a press under the precompile page taken as seen, never forced
    for(int button=0;button<3;++button){
        const auto presses=native_mouse_presses[button].load();
        if(fresh || page){entry->second[button]=presses;continue;}
        if(presses==entry->second[button])continue;
        entry->second[button]=presses;   // presses since the last update merge into one (NATIVE-INPUT-LATCH N3)
        const auto fields=reinterpret_cast<volatile uint8_t*>(mouse+8+8*button);
        if(!fields[0] && !fields[2]){fields[0]=1;fields[1]=0;fields[2]=1;
            const auto n=InterlockedIncrement64(&forced_updates);
            if(n<=64 || n%100==0){char row[200]{};sprintf_s(row,"{\"event\":\"native_input_latch\",\"tick\":%llu,\"device\":\"mouse\",\"object\":%llu,\"input\":%d,\"phys_down\":%s,\"forced\":%lld}\n",
                static_cast<unsigned long long>(GetTickCount64()),static_cast<unsigned long long>(mouse),button,native_mouse_down[button]?"true":"false",n);Log(row);}}
    }
}
void __fastcall NativeMouseDeviceUpdate(uintptr_t mouse,uintptr_t frame) noexcept {
    // 23891E0 is the genuine mouse device update: wheel latch, then (only when
    // the old renderer display object4961AB0 exists) the cursor position in
    // presentation units = client position * display extent198/19C / client
    // extent, then buttons from GetKeyboardState. Native execution has no such
    // object, so the original keeps position0,0 (Native897). Keep the original
    // for wheel/buttons and supply the position from the current native
    // presentation extent, the same unit the native UI target uses.
    mouse_device_update_original(mouse,frame);
    ApplyNativeMouseLatch(mouse);   // before the early returns (NATIVE-INPUT-LATCH N4)
    input_focus::PageMenuMouse(mouse);   // R12h's L7: no pressed edge nor wheel under the precompile page
    if(control->stop && control->closed)return;
    uintptr_t display{};
    if(!Read(image+0x4961AB0,&display,8) || display)return;
    // 1181 (I-0067 static reading, MENUS-MAP 51): the UI hit test (A5C010 -> A5B3F0 -> E63AA0 -> E65E70) reads the point
    // in the configured resolution S (image+46601E8/+46601EC), which display+198/19C carries in the original; the output
    // extent missed every button once S differed from it (Native1179: OK of the profile MessageBox never hit). Since I-0076 the
    // output follows S at the next engine frame, so the two differ only for that frame or under no-output-follow.flag.
    uint32_t configured[2]{};const bool use_configured=Read(image+0x46601E8,configured,sizeof(configured)) && configured[0] && configured[1];
    const auto width=use_configured?configured[0]:output_width,height=use_configured?configured[1]:output_height;HWND window{};
    if(!width || !height || !Read(mouse,&window,sizeof(window)) || !window)return;
    POINT cursor{};RECT client{};
    if(!GetCursorPos(&cursor) || !ScreenToClient(window,&cursor) || !GetClientRect(window,&client) ||
       client.right<=client.left || client.bottom<=client.top)return;
    const int32_t position[2]{
        static_cast<int32_t>(static_cast<float>(width)*static_cast<float>(cursor.x)/static_cast<float>(client.right-client.left)),
        static_cast<int32_t>(static_cast<float>(height)*static_cast<float>(cursor.y)/static_cast<float>(client.bottom-client.top))};
    std::memcpy(reinterpret_cast<void*>(mouse+0x20),position,sizeof(position));
    static LONG64 samples{};if(InterlockedIncrement64(&samples)<=4){char row[256]{};
        sprintf_s(row,"{\"event\":\"native_mouse_position_owned\",\"x\":%d,\"y\":%d,\"extent_width\":%u,\"extent_height\":%u,\"configured_resolution\":%s,\"output\":[%u,%u],\"legacy_display\":false}\n",position[0],position[1],width,height,use_configured?"true":"false",output_width,output_height);Log(row);}
}
uintptr_t __fastcall NativeUiPointerState(uintptr_t context,uintptr_t output) noexcept {
    // A60700 reads genuine CPU mouse/button/wheel state, then subtracts the
    // original GX target's viewport origin. Our presentation and movie buffer
    // both cover the complete client area with origin(0,0); no GX owner exists.
    if(!output)Refuse("native_ui_pointer_target_unavailable");
    // I-0113 tranche 19 (FINISH-LINE line 1): no longer refused before the output is open; nothing here reads the output's size
    // (the step review's L1: A60700 itself keeps the caller's position when it has no mouse).
    std::memset(reinterpret_cast<void*>(output+8),0,8);
    if(!reinterpret_cast<uint8_t(__fastcall*)(uintptr_t)>(image+0xA5B930)(context))return output;
    std::array<uintptr_t,3> handle{};
    reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,void*)>(image+0xA608E0)(context,handle.data());
    const auto mouse=reinterpret_cast<uintptr_t(__fastcall*)(void*)>(image+0xEB6CC0)(handle.data());
    reinterpret_cast<void(__fastcall*)(void*)>(image+0xEB6C10)(handle.data());
    if(!mouse)return output;
    uint8_t first{},second{};int32_t wheel{},position[2]{};
    if(!Read(mouse+8,&first,1) || !Read(mouse+0x10,&second,1) ||
       !Read(mouse+0x28,&wheel,4) || !Read(mouse+0x20,position,sizeof(position)))Refuse("native_ui_pointer_cpu_state_unreadable");
    const uint32_t buttons=(first?1u:0u)|(second?4u:0u);
    const auto scroll=static_cast<int16_t>(-(wheel/120));
    std::memcpy(reinterpret_cast<void*>(output),position,sizeof(position));
    std::memcpy(reinterpret_cast<void*>(output+8),&buttons,4);
    std::memcpy(reinterpret_cast<void*>(output+0xC),&scroll,2);
    ObserveUiPointerResult(context,output,reinterpret_cast<uintptr_t>(_ReturnAddress()));
    static LONG64 samples{};if(InterlockedIncrement64(&samples)<=8)
        Log("{\"event\":\"native_ui_pointer_cpu_state\",\"client_origin_x\":0,\"client_origin_y\":0,\"original_gpu_target\":false}\n");
    return output;
}
uintptr_t __fastcall NativeUiMovieViewport(uintptr_t movie,uintptr_t output) noexcept {
    // E53880's movie viewport work is CPU state. Its final adjustment queries
    // the old GX multi-target root, which does not exist in native execution.
    // The native presentation target defines the buffer and visible extent;
    // the actual movie retains its authored viewport/scale policy via60/68.
    struct Viewport {int32_t buffer_width,buffer_height,left,top,width,height;
        int32_t scissor_left,scissor_top,scissor_width,scissor_height;
        uint32_t flags;float scale_x,scale_y;};
    static_assert(sizeof(Viewport)==0x34);
    uintptr_t player{},table{},set{},get{};
    if(!output || !Read(movie+0x18,&player,8) || !player || !Read(player,&table,8) ||
       !Read(table+0x60,&set,8) || !set || !Read(table+0x68,&get,8) || !get)
        Refuse("native_ui_movie_viewport_contract");
    // I-0113 tranche 19 (FINISH-LINE line 1; i125-r6e2-gundyr, 2026-11-06 14:24: the title movie advanced before the window was
    // open): E53880 itself sets the engine's size S (image+46601E8/EC, its _DAT_1446601e8/ec), which the output follows from the
    // next engine frame (I-0076); until the output is open its size is S's. With neither, the output E53880 writes before its query
    // (zeros, a 1 x 1 extent, scale 1) and no set, counted.
    uint32_t size[2]{output_width,output_height};bool engine_size=false;
    if(!opened || !size[0] || !size[1]){uint32_t configured[2]{};
        if(Read(image+0x46601E8,configured,sizeof(configured)) && configured[0] && configured[1]){size[0]=configured[0];size[1]=configured[1];engine_size=true;}
        else{Viewport empty{};empty.width=1;empty.height=1;empty.scale_x=1;empty.scale_y=1;std::memcpy(reinterpret_cast<void*>(output),&empty,sizeof(empty));
            static LONG64 skipped{};const auto n=InterlockedIncrement64(&skipped);
            if(n<=8 || n%600==0){char row[160]{};sprintf_s(row,"{\"event\":\"native_ui_movie_viewport_skipped\",\"n\":%lld,\"opened\":%s}\n",
                static_cast<long long>(n),opened?"true":"false");Log(row);}
            return output;}}
    Viewport viewport{static_cast<int32_t>(size[0]),static_cast<int32_t>(size[1]),0,0,
        static_cast<int32_t>(size[0]),static_cast<int32_t>(size[1]),0,0,0,0,0,1,1};
    reinterpret_cast<void(__fastcall*)(uintptr_t,const Viewport*)>(set)(player,&viewport);
    viewport={};viewport.width=1;viewport.height=1;viewport.scale_x=1;viewport.scale_y=1;
    reinterpret_cast<void(__fastcall*)(uintptr_t,Viewport*)>(get)(player,&viewport);
    std::memcpy(reinterpret_cast<void*>(output),&viewport,sizeof(viewport));
    // The step review's L4: a viewport set from the engine's size S (the output not open) has its own count, rate-limited.
    static LONG64 samples{},engine_samples{};
    if(engine_size?[]{const auto n=InterlockedIncrement64(&engine_samples);return n<=8 || n%600==0;}():InterlockedIncrement64(&samples)<=8){char row[288]{};
        sprintf_s(row,"{\"event\":\"native_ui_movie_viewport\",\"buffer_width\":%d,\"buffer_height\":%d,\"width\":%d,\"height\":%d,\"flags\":%u,\"source\":\"%s\",\"original_gpu_target\":false}\n",
            viewport.buffer_width,viewport.buffer_height,viewport.width,viewport.height,viewport.flags,engine_size?"engine_size":"output");Log(row);}
    return output;
}
void __fastcall NativeRenderStep(uintptr_t service,uintptr_t) noexcept {
    kprofile::Mark(kprofile::RenderStepEntry);const kprofile::MarkOnExit render_step_exit{kprofile::RenderStepExit};
    try {
        if(control->stop && control->closed)return;
        BYTE enabled{};
        if(!Read(service+0xC8,&enabled,1))Refuse("native_render_step_service_unreadable");
        if(!enabled)return;
        if(const auto task=graphics_startup_task.load()){
            static uint32_t last_phase=~uint32_t{};uint32_t phase{},progress{};uintptr_t table{},callback{};BYTE pending{};
            if(!Read(task+0x50,&phase,4) || !Read(task+0x54,&progress,4) || !Read(task+0x58,&pending,1) ||
               !Read(task+0x10,&table,8) || phase>=19 || !Read(table+phase*16,&callback,8))Refuse("native_graphics_startup_state_unreadable");
            if(phase!=last_phase){last_phase=phase;char row[256]{};
                sprintf_s(row,"{\"event\":\"native_graphics_startup_state\",\"phase\":%u,\"progress\":%u,\"pending\":%u,\"callback_rva\":%llu}\n",
                    phase,progress,pending,static_cast<unsigned long long>(callback-image));Log(row);
            }
        }
        if(!opened || !native_resource_consumers || GetCurrentThreadId()!=owner_thread)Refuse("native_render_step_without_owner");
        native_ui_frame.reset();
        // I-0138 tranche 8h: D55190's gate (FUN_140d56da0, 14_scene_step_and_storage.inc ReadNativeDrawGate), read where it reads it;
        // a step whose collect ran follows the collect's reading (the frame drawn is the one collected), its own differing counted.
        // Closed, the step prepares no UI and EndFrame draws and presents nothing.
        const auto draw_gate=ReadNativeDrawGate();const int collect_gate=std::exchange(native_collect_gate,0);
        const bool draw=collect_gate?collect_gate==1:draw_gate.Open();
        NoteNativeDrawGate(!draw,draw_gate,collect_gate && draw!=draw_gate.Open());native_draw_skipped=!draw;
        uintptr_t main_task{};
        if(!Read(image+0x489D598,&main_task,8))Refuse("native_main_task_unreadable");
        if(main_task){
            static uint32_t last_phase=~uint32_t{};uint32_t phase{},progress{};uintptr_t table{},callback{},vtable{},graphics{};
            if(!Read(main_task,&vtable,8) || vtable!=image+0x29B08A8 || !Read(main_task+0x50,&phase,4) ||
               !Read(main_task+0x54,&progress,4) || !Read(main_task+0x10,&table,8) || phase>=25 ||
               !Read(table+phase*16,&callback,8) || !Read(main_task+0x160,&graphics,8))Refuse("native_main_task_contract");
            if(phase!=last_phase){last_phase=phase;char row[320]{};
                sprintf_s(row,"{\"event\":\"native_main_startup_state\",\"phase\":%u,\"progress\":%u,\"callback_rva\":%llu,\"graphics_task_matches\":%s}\n",
                    phase,progress,static_cast<unsigned long long>(callback-image),graphics==graphics_startup_task.load()?"true":"false");Log(row);
            }
        }
        // Native frame ownership replaces the original GPU-bound pump.
        //Initialize the genuine, already constructed game UI manager as soon
        //as its file opener is present. No fabricated manager or readiness.
        // E50230 publishes the 0x30-byte E4D490 container. Its +8 owns
        // the actual E57520 manager; the global is not the manager itself.
        uintptr_t ui_container{},ui_manager{},opener{},hal{};
        if(!Read(image+0x4798EF8,&ui_container,8))Refuse("native_ui_container_unreadable");
        if(ui_container){
            if(!Read(ui_container+8,&ui_manager,8) || !ui_manager ||
               !Read(ui_manager+0x968,&opener,8) || !Read(ui_manager+0x980,&hal,8))Refuse("native_ui_container_contract");
            // Factory creation belongs to the genuine CPU UI startup task.
        }
        // I-0088 P1: the display posted at the collect's entry is joined here; without a collect this frame (title, menus,
        // loading, inline-render.flag) the step prepares it inline as before.
        uint32_t movies{};bool joined{},session_found{};
        if(draw){
        if(native_ui_display.posted){kprofile::Mark(kprofile::UiPrepareEntry);joined=TakeNativeUiDisplay(movies);kprofile::Mark(kprofile::UiPrepareExit);}
        else if(hal){const std::lock_guard<std::timed_mutex> preparation(ui_frame_preparation_lock);
            std::shared_ptr<bootstrap::GameUiSession> session;
            {const std::lock_guard<std::mutex> lock(ui_sessions_lock);
                const auto found=ui_sessions.find(ui_manager);if(found!=ui_sessions.end())session=found->second;}
            // Do not hold the registry lock while actual movie CPU capture
            // locks are taken; the loader may import images on another worker.
            session_found=bool(session);
            if(session){kprofile::Mark(kprofile::UiPrepareEntry);movies=session->PrepareFrame(primitive_programs,serial+1,output_width,output_height);kprofile::Mark(kprofile::UiPrepareExit);
                if(auto packet=session->TakePreparedFrame())native_ui_frame=std::move(packet);   // moved: the session prepares the next one afresh
                for(const auto& report:session->TakeFilterReports())Log(report.c_str());}
        }
        // I-0137 tranche 7: why this frame would have no UI packet (02_frame_lifecycle.inc, NativeUiStepCause)
        native_ui_step_cause=joined?(native_ui_display_dropped?NativeUiStepCause::DisplayChanged:movies?NativeUiStepCause::MoviesWithoutPacket:NativeUiStepCause::NoMovie):
            !session_found?NativeUiStepCause::NoUiSession:movies?NativeUiStepCause::MoviesWithoutPacket:NativeUiStepCause::NoMovie;}
        rekindled_overlay::Step(output_width,output_height);   // I-0132 R12a: our pages' overlay, on the engine thread after the UI's frame (every step: its title music and sounds)
        if(draw){static LONG64 frames[2]{};if(movies && InterlockedIncrement64(&frames[joined])<=8){char row[224]{};   // the first 8 of each path
            sprintf_s(row,"{\"event\":\"native_ui_cpu_frame\",\"movies\":%u,\"draws_prepared\":%zu,\"ui_worker\":%s,\"original_gpu_target\":false,\"native_draw_submitted\":false}\n",
                movies,native_ui_frame?native_ui_frame->draws.size():0,joined?"true":"false");Log(row);}}
        static uintptr_t last_container=~uintptr_t{},last_manager=~uintptr_t{};
        if(ui_container!=last_container || ui_manager!=last_manager){
            last_container=ui_container;last_manager=ui_manager;char row[256]{};
            sprintf_s(row,"{\"event\":\"native_ui_bootstrap_state\",\"container\":%llu,\"manager\":%llu,\"opener_present\":%s}\n",
                static_cast<unsigned long long>(ui_container),static_cast<unsigned long long>(ui_manager),opener?"true":"false");Log(row);
        }
        //D55190's original render route calls DFDD20 then the old draw graph.
        //The native renderer already owns frame completion/presentation in
        //EndFrame. Preserve authored CPU colour intent separately; no original
        //GX object or GPU setter stream is consumed by native composition.
        uintptr_t settings{};
        if(!Read(image+0x4796298,&settings,8))Refuse("native_colour_source_unreadable");
        UpdateNativeOptions(settings);   // O2: the frame's one read of the graphics options
        if(settings){
            float amount{},scale{},offset{};
            // I-0101: a brightness input not readable or not finite keeps the previous colour settings (counted every frame until the
            // game writes the input again; +0x94 is not cleared then).
            const bool read=Read(settings+0x94,&amount,4) && Read(settings+0x98,&scale,4) && Read(image+0x3D7EE90,&offset,4);
            const float brightness=amount*scale+offset;
            if(!read || !std::isfinite(amount) || !std::isfinite(scale) || !std::isfinite(offset) || !std::isfinite(brightness))NoteNativeSkip("colour_input_invalid",settings);
            else{const std::lock_guard<std::mutex> lock(colour_settings_lock);
                native_colour_settings={2.2f,2.2f,2.2f,brightness,brightness,brightness};++native_colour_generation;
                *reinterpret_cast<float*>(settings+0x94)=0;}
        }
        if(native_colour_generation<=4)Log("{\"event\":\"native_render_step\",\"cpu_colour_intent\":true,\"presentation_owner\":\"native_end_frame\",\"legacy_draw_graph\":false,\"colour_pass_implemented\":false}\n");
    }catch(const std::exception& error){char row[512]{};
        sprintf_s(row,"{\"event\":\"native_render_step_exception\",\"message\":\"%s\"}\n",error.what());Log(row);
        Refuse("native_render_step_failed");}
}
void __fastcall EndFrame(void* manager) noexcept {
    kprofile::Mark(kprofile::EndFrameEntry);ds3native::cadence::Mark(ds3native::cadence::EngineEndEnter);
    try {
        // I-0138 tranche 8h: the step's gate readings end here, before any return; after the owner check, a step whose collect alone
        // read the gate is counted from that reading, and a step that read it nowhere (the render step disabled, no collect) computes
        // +0xCA now (8i: the original's step computes it every step once +0x98 is set) and ends a closed run; that step draws as
        // before tranche 8h (the original's D55190, disabled, would draw nothing: open).
        const bool skipped=std::exchange(native_draw_skipped,false),read=std::exchange(native_draw_gate_read,false);
        const int collect_gate=std::exchange(native_collect_gate,0);
        if(control->stop && control->closed) return;
        if(!opened || GetCurrentThreadId()!=owner_thread) Refuse("frame_end_without_native_owner");
        if(collect_gate)NoteNativeDrawGate(skipped,native_collect_reading,false);
        else if(!read){ReadNativeDrawGate();native_draw_gate_read=false;NoteNativeDrawGate(false,NativeDrawGate{},false);}
        // I-0111 tranche 2 (PCL, CADENCE-REFLEX-READY §5.6): the update and the render step of frame serial + 1 (the serial Clear is
        // about to take) are done. I-0138 tranche 8h: a step the original's draw gate left undrawn (the collect's reading, or the render
        // step's when it read the gate) has no frame: no render, no present and no pacing (the original's limiter skips its wait on
        // +0x27B; INFERRED for a step closed by +0x278 alone), no PCL marker (its frame's simulation goes on to the next drawn step), the screen keeping the last drawn frame.
        if(!skipped)ds3native::streamline::Marker(ds3native::streamline::PclMarker::SimulationEnd,serial+1);
        FinishNativeUiDisplay();   // I-0088 P1: a display the render step did not join ends inside its frame
        uintptr_t clock{};
        if(!Read(image+0x489DD10,&clock,8) || !clock) Refuse("frame_clock_unavailable");
        reinterpret_cast<Phase>(image+0xEEE190)(reinterpret_cast<void*>(clock));
        const auto receiver=reinterpret_cast<uintptr_t>(manager);
        *reinterpret_cast<int*>(receiver+0xBC)=0;
        *reinterpret_cast<int*>(receiver+0xB8)=0;
        if(control->stop) Close(); else if(!skipped){native_end_frame_clear=true;Clear();native_end_frame_clear=false;}
        LARGE_INTEGER tick{}; QueryPerformanceCounter(&tick);
        *reinterpret_cast<LONGLONG*>(clock+0x48)=tick.QuadPart;ds3native::cadence::MarkValue(ds3native::cadence::EnginePostDone,tick.QuadPart);
        native_cloth_frames.fetch_add(1,std::memory_order_relaxed);   // I-0134
        if(++lifecycle_frames<=8) Log("{\"event\":\"native_frame_lifecycle\",\"cpu_clock_preserved\":true}\n");
        // I-0111 tranche 2 (display-clock.flag, CADENCE-REFLEX-READY §2, §8): the frame-start wait before DS3's limiter (the next
        // timeline task, on this thread: its fields are read while it does not run). Its deadline, read only: the previous exit
        // [clock+0x28] plus the period [clock+0x18] (seconds) times DS3's counter frequency [0x144929E00]; nothing is written.
        // Step review L1: DS3's counter frequency must be this process's (the deadline is compared with QueryPerformanceCounter).
        // I-0111 tranche 2 (Reflex, D-0240, CADENCE-REFLEX-READY §5.5): Reflex's sleep every frame once its options are sent (it
        // returns at once Off: step review L1). With Reflex On or Boost it is the frame-start wait, and the display clock is not
        // engaged (Clear: the worker waits the object and paces as without the clock, step review M2); Off, the display clock's wait.
        if(!control->stop && !skipped)ds3native::streamline::ReflexSleep(serial+1);
        if(!control->stop && !skipped && !native_reflex_on && NativeDisplayClockRequested()){LONGLONG exit{},frequency{};float period{};int64_t deadline{};LARGE_INTEGER own{};QueryPerformanceFrequency(&own);
            if(Read(clock+0x28,&exit,8) && Read(clock+0x18,&period,4) && Read(image+0x4929E00,&frequency,8) && exit>0 && frequency==own.QuadPart &&
               std::isfinite(period) && period>0 && period<1)deadline=exit+int64_t(double(period)*double(frequency)+0.5);
            (void)renderer.WaitDisplayReady(deadline);
            static uint64_t display_rows{};if(!(++display_rows%600)){const auto c=renderer.TakeDisplayClockCounts();char row[320]{};
                sprintf_s(row,"{\"event\":\"native_display_clock\",\"frames\":%llu,\"engaged\":%llu,\"released\":%llu,\"stored\":%llu,\"timeouts\":%llu,\"owed_taken\":%llu,\"skipped\":%llu,\"probed\":%llu,\"waited_ms\":%.3f,\"lead_ms\":%.3f,\"period_ms\":%.4f}\n",
                    static_cast<unsigned long long>(c.waits),static_cast<unsigned long long>(c.engaged),static_cast<unsigned long long>(c.released),static_cast<unsigned long long>(c.stored),
                    static_cast<unsigned long long>(c.timeouts),static_cast<unsigned long long>(c.owed_taken),static_cast<unsigned long long>(c.skipped),static_cast<unsigned long long>(c.probed),
                    c.engaged?c.waited_ms/double(c.engaged):0.0,c.released?c.lead_ms/double(c.released):0.0,double(period)*1000.0);Log(row);}}
        // PCL: frame serial + 1's simulation starts (after the frame-start wait; DS3's limiter, the next timeline task, follows).
        if(!control->stop && !skipped)ds3native::streamline::Marker(ds3native::streamline::PclMarker::SimulationStart,serial+1);
    } catch(const std::exception& e) {NativeFailure(e.what());}
}
}  // namespace
