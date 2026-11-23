// Native CPU owners of engine objects: env regions, decal and paint requests, the scene tree, scene contexts and views, the
// scene camera, face images, model streams and the model pose types. Part of game_bootstrap.cpp's single translation unit
// (the order is there): it relies on every earlier part (02_frame_lifecycle.inc before it).
#pragma once
#include "02_frame_lifecycle.inc"
namespace {
NativeEnvRegionCpuRegistry& EnvRegionCpuRegistry() {
    if(env_region_cpu_closed)Refuse("native_env_region_cpu_closed");
    if(env_region_cpu)return *env_region_cpu;
    auto owner=std::make_unique<NativeEnvRegionCpuRegistry>();uintptr_t heap_table{},allocate{},root{},prior[2]{};BYTE enabled{};
    if(!Read(image+0x4961B80,&owner->heap,8) || !owner->heap ||
       !Read(owner->heap,&heap_table,8) || !Read(heap_table+0x48,&allocate,8) || !allocate)
        Refuse("native_env_region_cpu_heap");
    // The native scene root's 486A0 already ran the actual 65370 at root+0x42B0
    // (CPU vectors, default region, lock). The mixed 65790 programme/buffer
    // initialization is never invoked; native installs its CPU arrays below.
    if(!Read(image+0x4961B50,&root,8) || !root || root!=native_scene_root)Refuse("native_env_region_cpu_root");
    const auto manager=root+0x42B0;owner->manager=manager;
    if(!Read(manager+0x58,&prior[0],8) || !Read(manager+0x2C8,&prior[1],8) || prior[0] || prior[1] ||
       !Read(manager+0x10,&enabled,1) || enabled)Refuse("native_env_region_cpu_manager_not_pristine");
    const auto alloc=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,size_t)>(allocate);
    owner->buckets=alloc(owner->heap,size_t(owner->count)*0x18);
    owner->packed=alloc(owner->heap,size_t(owner->count)*0x130);
    if(!owner->buckets || !owner->packed)Refuse("native_env_region_cpu_allocation");
    std::memset(reinterpret_cast<void*>(owner->buckets),0,size_t(owner->count)*0x18);
    std::memset(reinterpret_cast<void*>(owner->packed),0,size_t(owner->count)*0x130);
    for(uint32_t n=0;n<owner->count;++n){
        *reinterpret_cast<uint64_t*>(owner->buckets+uint64_t(n)*0x18+8)=0x100000011ull;
        *reinterpret_cast<BYTE*>(owner->buckets+uint64_t(n)*0x18+0x12)=1;
    }
    *reinterpret_cast<uintptr_t*>(manager+0x58)=owner->buckets;
    *reinterpret_cast<uintptr_t*>(manager+0x2C8)=owner->packed;
    // This enables the genuine allocator/traversal of root+0x42B0 only; its
    // GPU slots remain nil.
    *reinterpret_cast<BYTE*>(manager+0x10)=1;
    env_region_cpu=std::move(owner);
    Log("{\"event\":\"native_env_region_cpu_registry\",\"count\":512,\"actual_cpu_constructor\":true,\"original_renderer_published\":false,\"native_ibl_draw_ready\":false}\n");
    return *env_region_cpu;
}
uintptr_t EnvRegionCpuRecord(uintptr_t wrapper,bool fallback) {
    if(!env_region_cpu || !env_region_cpu->wrappers.contains(wrapper))
        Refuse("native_env_region_cpu_foreign_wrapper");
    const auto& owner=*env_region_cpu;uint32_t handle{};uintptr_t table{},record{};uint16_t generation{};
    if(!Read(wrapper,&table,8) || table!=image+0x3D42148 || !Read(wrapper+8,&handle,4))
        Refuse("native_env_region_cpu_wrapper_contract");
    if(handle==UINT32_MAX){if(fallback)return owner.manager+0x80;return 0;}
    const auto index=handle&0xFFFFu;
    if(handle!=owner.wrappers.at(wrapper) || index>=owner.count ||
       !Read(owner.buckets+uint64_t(index)*0x18+0x10,&generation,2) || generation!=(handle>>16) ||
       !Read(owner.buckets+uint64_t(index)*0x18,&record,8) || !record)
        {   // 1258 RCA witness (quit in game): the slot state and the caller chain of the stale handle before the refusal.
            uint16_t slot_generation{};uintptr_t slot_record{};if(index<owner.count){Read(owner.buckets+uint64_t(index)*0x18+0x10,&slot_generation,2);Read(owner.buckets+uint64_t(index)*0x18,&slot_record,8);}
            void* frames[14]{};const auto depth=RtlCaptureStackBackTrace(1,14,frames,nullptr);std::string stack;
            for(USHORT i=0;i<depth;++i){const auto at=reinterpret_cast<uintptr_t>(frames[i]);char cell[24]{};sprintf_s(cell,"%s%lld",stack.empty()?"":",",at>=image && at<image+0x6000000?static_cast<long long>(at-image):-1ll);stack+=cell;}
            char row[640]{};sprintf_s(row,"{\"event\":\"native_env_region_cpu_stale\",\"handle\":%u,\"registered\":%u,\"slot_generation\":%u,\"slot_record\":%s,\"fallback\":%s,\"thread\":%lu,\"stack\":[%s]}\n",
                handle,owner.wrappers.contains(wrapper)?owner.wrappers.at(wrapper):0u,slot_generation,slot_record?"true":"false",fallback?"true":"false",GetCurrentThreadId(),stack.c_str());Log(row);
            Refuse("native_env_region_cpu_generation");}
    return record;
}
uintptr_t __fastcall RequestNativeEnvRegionCpu(uintptr_t excluded_manager,uintptr_t descriptor) noexcept {
    try {
        std::unique_lock hold(env_region_cpu_lock);uintptr_t root{};
        // I-0140 tranche 28 (D-0301): the menu views' region adder D0C2F0 (its call at 140D0C903) as well as the MSB region
        // producer (D0A890). The original's manager is on at every D0C2F0 call (OBSERVED, i140-t28d-witness-orig); natively it is
        // turned on by its first request, so the menu views' call may be that first one (at the title: 10 of 11 natively).
        // A menu call outside the scope (another root or manager than the native scene root's, none seen) answers 0, and so does
        // any of the three callers' after the registry's close: the original's answer while its manager is off (D0A890 tests it;
        // D0C2F0 and D0CA20 hand it to 65320, answered 0 below; the second step review's L1); a row, its first 8.
        // The final review's M2: 65D60's third caller, D0CA20 (from 936700, once its four loaded resources are ready: two textures
        // and a gparam, INFERRED a loaded env-map set; never met in the journals), adds one region {0x11, 1} through the same CPU
        // record, which the region consumers read by its key as an MSB region's; it was refused (the game closed) before (READ: its
        // disassembly, the call at 140D0CFAD).
        const uintptr_t caller=reinterpret_cast<uintptr_t>(_ReturnAddress());const bool menu=caller==image+0xD0C908,set=caller==image+0xD0CFB2;
        const bool scoped=Read(image+0x4961B50,&root,8) && root && root==native_scene_root && excluded_manager==root+0x42B0;
        if((menu && !scoped) || (scoped && env_region_cpu_closed && (menu || set || caller==image+0xD0A890))){static LONG64 rows{};
            if(InterlockedIncrement64(&rows)<=8)Log(scoped?"{\"event\":\"native_env_region_cpu_request_answered_0\",\"reason\":\"closed\"}\n":"{\"event\":\"native_env_region_cpu_request_answered_0\",\"reason\":\"menu_scope\"}\n");
            return 0;}
        if((caller!=image+0xD0A890 && !menu && !set) || !scoped)Refuse("native_env_region_cpu_request_scope");
        std::array<BYTE,0x170> input{};
        if(!Read(descriptor,input.data(),input.size()))Refuse("native_env_region_cpu_descriptor");
        const auto manager=EnvRegionCpuRegistry().manager;
        // The final review's M1: the registry's lock is not held across 65D60, which takes the manager's lock +0x2A0; 664D0 (the
        // release, D09F47's and NativeMenuRegionRelease's) takes +0x2A0 then this lock through 652A0's replacement. Held across
        // the call, the two orders were inverse (a loader thread's add against the engine thread's menu teardown). The call is
        // counted in env_region_cpu_calls until its wrapper is emplaced: the close waits for it before 66640 (its step review's
        // M1), which then retires that wrapper as any other; the caller's 65320 on it that follows is answered 0 once the registry
        // is closed (the second step review's M1).
        struct InFlight {InFlight(){env_region_cpu_calls.fetch_add(1,std::memory_order_acq_rel);}
            ~InFlight(){env_region_cpu_calls.fetch_sub(1,std::memory_order_acq_rel);}} in_flight;
        hold.unlock();
        const auto wrapper=env_region_request_original(manager,descriptor);
        hold.lock();
        if(!wrapper)return 0;
        if(!env_region_cpu || env_region_cpu->manager!=manager)Refuse("native_env_region_cpu_retired_during_request");
        auto& owner=*env_region_cpu;uint32_t handle{};
        if(!Read(wrapper+8,&handle,4) || !owner.wrappers.emplace(wrapper,handle).second)
            Refuse("native_env_region_cpu_duplicate_wrapper");
        const auto record=EnvRegionCpuRecord(wrapper,false);std::array<BYTE,0x170> prepared{};
        if(!record || !Read(record,prepared.data(),prepared.size()))Refuse("native_env_region_cpu_result");
        // Preserve actual D0B8A0/650F0/66330/66FA0 CPU input and normalization.
        // The existing native/world Banks::Env and pass producers remain the
        // native renderer route; these current CPU records are its ingress.
        // A row per admission, its first 256 (menu views now add one at each creation: the final review's L6), with the thread.
        static LONG64 admitted{};if(InterlockedIncrement64(&admitted)<=256){char row[360]{};
            sprintf_s(row,"{\"event\":\"native_env_region_cpu_admitted\",\"handle\":%u,\"actual_cpu_record\":true,\"actual_msb_region_producer\":%s,\"menu_region\":%s,\"set_region\":%s,\"native_ibl_draw_ready\":false,\"legacy_gpu_work\":false,\"thread\":%lu}\n",handle,menu || set?"false":"true",menu?"true":"false",set?"true":"false",GetCurrentThreadId());Log(row);}
        return wrapper;
    }catch(...){Refuse("native_env_region_cpu_request_failed");}
}
uintptr_t __fastcall GetNativeEnvRegionCpu(uintptr_t wrapper) noexcept {
    // From the moment the close starts (its 66640 then retires the wrappers one by one, and the registry goes), a holder's lookup
    // gets the closed manager's default record +0x80, as 652D0's own answer for a stale wrapper, without reading the wrapper
    // (the third and fourth step reviews' M2, M1). A live wrapper gets it too during the close: 66640 and 664D0 read their records
    // from +0x58 themselves, never through 652D0 (READ). Only the native scene root's manager is ever admitted (the registry's
    // constructor), so the answer is that manager's; it is not answered past the root's own destructor (65690), which the module
    // does not outlive in play.
    try {const std::lock_guard hold(env_region_cpu_lock);
        if(env_region_cpu_closed && env_region_cpu_closed_manager)return env_region_cpu_closed_manager+0x80;
        return EnvRegionCpuRecord(wrapper,true);}
    catch(...){Refuse("native_env_region_cpu_lookup_failed");}
}
BYTE __fastcall SetNativeEnvRegionCpuMetadata(uintptr_t wrapper,uintptr_t metadata) noexcept {
    try {
        const std::lock_guard hold(env_region_cpu_lock);
        // D0C2F0 and D0CA20 hand 65320 whatever 65D60 returned, 0 included (its table full): the original would read null +8;
        // answered 0. After the registry's close the three callers' 65320 (D0A890's returns at 140D0A8C1) is answered 0 before any
        // read: a request in flight at the close returns a wrapper 66640 has retired (the second step review's M1); none of the
        // three uses 65320's answer.
        if(const auto at=reinterpret_cast<uintptr_t>(_ReturnAddress());
           ((!wrapper || env_region_cpu_closed) && (at==image+0xD0C928 || at==image+0xD0CFD5)) || (env_region_cpu_closed && at==image+0xD0A8C1))return 0;
        if(!EnvRegionCpuRecord(wrapper,false))return 0;
        uint64_t value{};uint32_t handle{};
        if(!Read(metadata,&value,8) || !Read(wrapper+8,&handle,4))Refuse("native_env_region_cpu_metadata");
        *reinterpret_cast<uint64_t*>(env_region_cpu->buckets+uint64_t(handle&0xFFFFu)*0x18+8)=value;
        return 1;
    }catch(...){Refuse("native_env_region_cpu_metadata_failed");}
}
// 664D0(manager, wrapper), the region release (D09F00's loop at D09F47, the map's regions; NativeMenuRegionRelease, the menu
// views'): it reads the manager's +0x10 (off: answers 0), takes +0x2A0, finds the wrapper in +0x18 and retires it through slot 0
// (652A0's replacement, this registry's lock) (READ: its disassembly). Every call on the native scene root's manager is counted in
// env_region_cpu_calls, so the close waits for it before 66640 and the frees of the manager's vectors and lock; after the close it
// answers 0, as 664D0 itself with the manager off (the second step review's M2). No registry lock is held across it.
BYTE __fastcall ReleaseNativeEnvRegionCpu(uintptr_t manager,uintptr_t wrapper) noexcept {
    try {
        bool counted{};
        {const std::lock_guard hold(env_region_cpu_lock);
            if(native_scene_root && manager==native_scene_root+0x42B0){if(env_region_cpu_closed)return 0;
                env_region_cpu_calls.fetch_add(1,std::memory_order_acq_rel);counted=true;}}
        struct InFlight {bool counted;~InFlight(){if(counted)env_region_cpu_calls.fetch_sub(1,std::memory_order_acq_rel);}} in_flight{counted};
        return env_region_release_original(manager,wrapper);
    }catch(...){Refuse("native_env_region_cpu_release_failed");}
}
// 66640(manager), the clear-all (void: its last instruction jumps to the lock's leave, READ): the game's callers are D0D060 (the
// set's removal, 936660's) and 65B70 (65690's, the manager's destructor). Counted as 664D0 on the native scene root's manager;
// after the close a no-op: the manager's vectors are freed and zeroed, its lock reset (the third step review's M1).
void __fastcall ClearNativeEnvRegionCpu(uintptr_t manager) noexcept {
    try {
        bool counted{};
        {const std::lock_guard hold(env_region_cpu_lock);
            if(native_scene_root && manager==native_scene_root+0x42B0){if(env_region_cpu_closed)return;
                env_region_cpu_calls.fetch_add(1,std::memory_order_acq_rel);counted=true;}}
        struct InFlight {bool counted;~InFlight(){if(counted)env_region_cpu_calls.fetch_sub(1,std::memory_order_acq_rel);}} in_flight{counted};
        env_region_clear_original(manager);
    }catch(...){Refuse("native_env_region_cpu_clear_failed");}
}
uintptr_t __fastcall DestroyNativeEnvRegionCpuWrapper(uintptr_t wrapper,BYTE flags) noexcept {
    try {
        const std::lock_guard hold(env_region_cpu_lock);
        if(flags || !env_region_cpu || !env_region_cpu->wrappers.contains(wrapper))
            Refuse("native_env_region_cpu_wrapper_retirement_scope");
        // 1260 (Native1259): the release 664D0 clears the slot's record, then destroys the wrapper through its slot 0
        // (652A0) and frees it. 652A0 itself checks nothing: only the wrapper's identity is required here (the record is
        // already gone on that path), and the retired wrapper leaves the registry (its memory is freed right after).
        uintptr_t table{};if(!Read(wrapper,&table,8) || table!=image+0x3D42148)Refuse("native_env_region_cpu_wrapper_contract");
        const auto result=env_region_destroy_original(wrapper,0);env_region_cpu->wrappers.erase(wrapper);
        static LONG64 retired{};if(InterlockedIncrement64(&retired)<=4)Log("{\"event\":\"native_env_region_cpu_wrapper_retired\"}\n");
        return result;
    }catch(...){Refuse("native_env_region_cpu_wrapper_retirement_failed");}
}
void CloseNativeEnvRegions() {
    // The final review's M1: 66640 takes the manager's lock +0x2A0, then this lock through 652A0's replacement for each wrapper;
    // it runs without this lock held, as 664D0's callers, once the registry is closed (new requests, 65320, 664D0 and 66640 on
    // it answered 0 or a no-op; 652D0 the default record) and the calls already in flight (env_region_cpu_calls: a request's
    // 65D60, every 664D0 and 66640 on it) have finished (its step review's M1). The manager is turned off (+0x10) before its
    // vectors are freed and zeroed (the destructor's 65B70 and 65690 free only what is left): the region readers
    // (CollectNativeRegionSnapshot, ProduceNativeMenuEnvRegions) take nothing from a manager that is off (the final review's L6).
    std::unique_lock hold(env_region_cpu_lock);env_region_cpu_closed=true;
    if(!env_region_cpu)return;
    const auto manager=env_region_cpu->manager;env_region_cpu_closed_manager=manager;   // 652D0's answer from here on
    if(!env_region_clear_original)Refuse("native_env_region_cpu_clear_unhooked");
    hold.unlock();
    while(env_region_cpu_calls.load(std::memory_order_acquire))SwitchToThread();
    // 66640 itself, through its trampoline (the hook is a no-op once closed).
    env_region_clear_original(manager);
    hold.lock();
    *reinterpret_cast<BYTE*>(manager+0x10)=0;
    env_region_cpu->wrappers.clear();
    FreeModelPrefix(env_region_cpu->buckets,env_region_cpu->heap);
    FreeModelPrefix(env_region_cpu->packed,env_region_cpu->heap);
    for(const auto offset:{0x18u,0x38u,0x2F8u}){uintptr_t bytes{},heap{};
        if(!Read(manager+offset,&bytes,8) || !Read(manager+offset+0x18,&heap,8))
            Refuse("native_env_region_cpu_vector_retirement");
        if(bytes)FreeModelPrefix(bytes,heap);
        std::memset(reinterpret_cast<void*>(manager+offset),0,0x18);   // begin, end, capacity: 65690 and 66640 skip an empty vector
    }
    *reinterpret_cast<uintptr_t*>(manager+0x58)=0;*reinterpret_cast<uintptr_t*>(manager+0x2C8)=0;   // freed above
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x17CC360)(manager+0x2A0);
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x17CC330)(manager+0x2A0);
    env_region_cpu.reset();
    Log("{\"event\":\"native_env_region_cpu_retired\",\"actual_cpu_list_retirement\":true,\"legacy_gpu_retired\":false}\n");
}
void __fastcall ConstructNativeDecalCpuRegistry(uintptr_t receiver,uintptr_t input) noexcept {
    try {
        // 4FEC0 already ran the genuine 502E0 constructor. Preserve that CPU
        // object, its real lock/vectors and original class identity. 50720's
        // native responsibility is its callback and handle registry only.
        NativeDecalCpuRegistry owned;uintptr_t table{},prior_observer{},prior_buckets[2]{},heap_table{},allocate{};
        if(!Read(receiver,&table,8) || table!=image+0x3D20460 ||
           !Read(receiver+0x18,&owned.heap,8) || !owned.heap ||
           !Read(receiver+0x88,&prior_observer,8) || prior_observer ||
           !Read(receiver+0x3F0,prior_buckets,sizeof(prior_buckets)) || prior_buckets[0] || prior_buckets[1] ||
           !Read(input,&owned.observer,8) || !Read(input+8,&owned.count,4) ||
           !Read(owned.heap,&heap_table,8) || !Read(heap_table+0x48,&allocate,8) || !allocate)
            Refuse("native_decal_cpu_construction_contract");
        if(!owned.count)owned.count=512;
        if(owned.count>65535)Refuse("native_decal_cpu_handle_range");
        if(owned.observer)Refuse("native_decal_external_observer_unowned");
        {
            owned.observer=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(8,8,owned.heap);
            if(!owned.observer)throw std::bad_alloc();
            *reinterpret_cast<uintptr_t*>(owned.observer)=image+0x3D5B7C0;
        }
        const auto allocate_cpu=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,size_t)>(allocate);
        owned.buckets=allocate_cpu(owned.heap,size_t(owned.count)*8);
        if(!owned.buckets){FreeModelPrefix(owned.observer,owned.heap);throw std::bad_alloc();}
        owned.generations=allocate_cpu(owned.heap,size_t(owned.count)*2);
        if(!owned.generations){FreeModelPrefix(owned.buckets,owned.heap);FreeModelPrefix(owned.observer,owned.heap);throw std::bad_alloc();}
        std::memset(reinterpret_cast<void*>(owned.buckets),0,size_t(owned.count)*8);
        std::memset(reinterpret_cast<void*>(owned.generations),0,size_t(owned.count)*2);
        // All of 280..3D7 are the excluded legacy GPU resources and states.
        // 502E0 leaves some of the later state slots uninitialized. Native
        // owns their absence; no fake object or legacy readiness is published.
        std::memset(reinterpret_cast<void*>(receiver+0x280),0,0x158);
        {std::lock_guard hold(decal_cpu_lock);
            if(!decal_cpu_registries.emplace(receiver,owned).second)Refuse("native_decal_cpu_duplicate_owner");}
        *reinterpret_cast<uintptr_t*>(receiver+0x88)=owned.observer;
        *reinterpret_cast<uintptr_t*>(receiver+0x3F0)=owned.buckets;
        *reinterpret_cast<uintptr_t*>(receiver+0x3F8)=owned.generations;
        *reinterpret_cast<uint32_t*>(receiver+0x400)=owned.count;
        char row[256]{};sprintf_s(row,"{\"event\":\"native_decal_cpu_registry\",\"count\":%u,\"actual_cpu_constructor\":true,\"legacy_gpu_created\":false,\"native_decal_rendering\":false}\n",owned.count);Log(row);
    }catch(...){Refuse("native_decal_cpu_construction_failed");}
}
void __fastcall RetireNativeDecalCpuRegistry(uintptr_t receiver) noexcept {
    try {
        NativeDecalCpuRegistry owned;
        {std::lock_guard hold(decal_cpu_lock);const auto found=decal_cpu_registries.find(receiver);
            if(found==decal_cpu_registries.end())Refuse("native_decal_cpu_foreign_retirement");owned=found->second;}
        std::array<uintptr_t,43> old_gpu{};uintptr_t heap{},observer{},buckets[2]{},geometry[2]{},extra{};uint32_t count{};
        if(!Read(receiver+0x280,old_gpu.data(),sizeof(old_gpu)) ||
           !Read(receiver+0x18,&heap,8) || heap!=owned.heap ||
           !Read(receiver+0x88,&observer,8) || observer!=owned.observer ||
           !Read(receiver+0x3F0,buckets,sizeof(buckets)) || buckets[0]!=owned.buckets || buckets[1]!=owned.generations ||
           !Read(receiver+0x400,&count,4) || count!=owned.count ||
           !Read(receiver+0x68,geometry,sizeof(geometry)) || geometry[1]<geometry[0] ||
           (geometry[1]-geometry[0])%8 || (geometry[1]-geometry[0])/8>owned.count ||
           !Read(receiver+0x438,&extra,8) || extra)
            Refuse("native_decal_cpu_retirement_contract");
        for(const auto resource:old_gpu)if(resource)Refuse("native_decal_legacy_gpu_present");
        for(auto at=geometry[0];at!=geometry[1];at+=8){uintptr_t object{},table{},cache{},vector{},begin{};uint32_t handle{};
            if(!Read(at,&object,8) || !object || !Read(object,&table,8) || (table!=image+0x3D5B8D8 && table!=image+0x3D5B978) ||
               !Read(object+0x138,&handle,4) || !owned.requests.contains(handle) ||
               !Read(object+0x130,&cache,8) || (table==image+0x3D5B978 && cache))Refuse("native_decal_cpu_retirement_foreign_request");   // kind 1: +0x130 cleared by 51BF0
            // 29C60's unfilled cache has no element retirement; its actual
            // vectors/heap wrapper are paired CPU allocations (source617).
            if(cache && (!Read(cache+8,&vector,8) || !vector || !Read(vector,&begin,8) || begin))
                Refuse("native_decal_cpu_cache_retirement_not_owned");
        }
        // Source524/604/605/617: 51860 retires real request objects through
        // 19B1B0/19B5F0, their empty CPU cache and resets the true buckets.
        // 51780's GPU branches and all other GPU retirement
        // branches are skipped because their actual slots are nil. 505E0 then
        // retires the real CPU observer, arrays, lock and vectors. The genuine
        // caller remains responsible for freeing the 540-byte object itself.
        decal_retire_original(receiver);
        {std::lock_guard hold(decal_cpu_lock);decal_cpu_registries.erase(receiver);}
        size_t kind1{};for(const auto& [h,request]:owned.requests){(void)h;kind1+=request.kind==1;}
        char row[256]{};sprintf_s(row,"{\"event\":\"native_decal_cpu_registry_retired\",\"cpu_requests\":%zu,\"kind1_requests\":%zu,\"legacy_gpu_retired\":false}\n",owned.requests.size(),kind1);Log(row);
    }catch(...){Refuse("native_decal_cpu_retirement_failed");}
}
void __fastcall QueryNativeDecalCpuWorld(uintptr_t world,uintptr_t task) noexcept {
    try {
        if(native_decal_query_scope){const auto& scope=*native_decal_query_scope;
            uintptr_t world_table{},task_table{},manager{},request{},callback{};BYTE deferred{};
            std::array<uintptr_t,2> layers{};
            if(!Read(world,&world_table,8) || world_table!=image+0x3D20398 ||
               !Read(task,&task_table,8) || task_table!=image+0x3D5B9C8 ||
               !Read(task+0x12,&deferred,1) || deferred ||
               !Read(task+0x80,&manager,8) || manager!=scope.manager ||
               !Read(task+0x88,&request,8) || request!=scope.descriptor ||
               !Read(task+0x90,&callback,8) || callback!=scope.callback ||
               !Read(world+0x10,layers.data(),sizeof(layers)) || layers[1]<layers[0] ||
               (layers[1]-layers[0])%8 || (layers[1]-layers[0])/8>4096)
                Refuse("native_decal_cpu_query_contract");
            // Source610/615: task12=0 selects the synchronous CPU traversal,
            // never C1FB0's old-renderer deferred work queue. 50120 is the
            // genuine default predicate, not a replacement accepting stub.
            decal_world_query_original(world,task);
            native_decal_query_scope->layers=static_cast<uint32_t>((layers[1]-layers[0])/8);
            ++native_decal_query_scope->completed;return;
        }
        // Keep the pre-existing CPU call path outside this new transaction.
        decal_world_query_original(world,task);
    }catch(...){Refuse("native_decal_cpu_world_query_failed");}
}
// DECALS-WAVE1 D1 (decal-observe.flag): the decal programmes' bytecode once (the registry's, under the manager's tables: +0x98, the
// pixel programmes, and +0x210, the vertex ones 198910 sets through 14002B7E0), next to the trace as decal-programme-<i>.dxbc and
// decal-vertex-<i>.dxbc, for their reflection. At the programme selection and at the first observed request.
void DumpNativeDecalProgrammes(const NativeDecalCpuRegistry& owner) {
    if(!control)return;const auto directory=JournalSibling(L"");
    uint32_t written{};for(size_t slot=0;slot<owner.programmes.size();++slot){std::shared_ptr<const NativeShader> shader;
        {const std::lock_guard shader_hold(shader_lock);if(const auto found=shaders.find(owner.programmes[slot]);found!=shaders.end())shader=found->second;}
        if(shader){std::ofstream file(directory+L"decal-programme-"+std::to_wstring(slot)+L".dxbc",std::ios::binary);file.write(reinterpret_cast<const char*>(shader->bytes.data()),shader->bytes.size());written+=bool(file);}}
    // the 14 "constant descriptions" (+0x210): 198910 sets its vertex programme from them (14002B7E0)
    for(size_t slot=0;slot<owner.constant_descriptions.size();++slot){std::shared_ptr<const NativeShader> shader;
        {const std::lock_guard shader_hold(shader_lock);if(const auto found=shaders.find(owner.constant_descriptions[slot]);found!=shaders.end())shader=found->second;}
        if(shader){std::ofstream file(directory+L"decal-vertex-"+std::to_wstring(slot)+L".dxbc",std::ios::binary);file.write(reinterpret_cast<const char*>(shader->bytes.data()),shader->bytes.size());written+=bool(file);}}
    char row[96]{};sprintf_s(row,"{\"event\":\"decal_observe_programmes\",\"written\":%u}\n",written);Log(row);}
std::string DecalObjectRow(uintptr_t object);   // decal-observe (below)
std::string HexBytes(const void* data,size_t bytes);
// DECALS-PAINT-READY rung 0 (decal-observe.flag, native): a completed kind-1 (paint) request: its descriptor (the 0x118 bytes, the box
// floats, kind, projection, the texture objects d+0x48..+0x78 with their vtable, native residency and size), and the manager's paint
// queue +0x20..+0x28 (0x30-byte entries {receiver, handle, +0x18 type}) with, once per receiver, its slots +0x20..+0x38, the draw
// object +0x40, the bloodmask index +0x58, the flags +0x5C/+0x78/+0x79 and the slot-0 target's +0x50 (initialised) and creation
// descriptor +0x60..+0x7F; and the per-frame budget +0x408. Called under decal_cpu_lock.
void ObserveNativePaintRequest(uintptr_t manager,uint32_t handle,const std::array<BYTE,0x118>& d) {
    static LONG64 observed{};const auto n=InterlockedIncrement64(&observed);if(n>24 && n%50)return;
    static std::set<uintptr_t> seen;
    float box[13]{};std::memcpy(box,d.data(),sizeof(box));uint32_t kind{},projection{};std::memcpy(&kind,d.data()+0x38,4);std::memcpy(&projection,d.data()+0x3C,4);
    std::string row="{\"event\":\"decal_observe_paint_request\",\"n\":"+std::to_string(n)+",\"handle\":"+std::to_string(handle)+",\"box\":[";
    for(int i=0;i<13;++i){char c[32]{};sprintf_s(c,"%s%g",i?",":"",box[i]);row+=c;}
    row+="],\"kind\":"+std::to_string(kind)+",\"projection\":"+std::to_string(projection)+",\"textures\":{";
    bool first=true;for(uint32_t at=0x48;at<=0x78;at+=8){uintptr_t texture{},table{};std::memcpy(&texture,d.data()+at,8);if(!texture)continue;Read(texture,&table,8);
        const auto record=textures.Acquire(texture);char c[160]{};sprintf_s(c,"%s\"%X\":[\"%llX\",%s,%u,%u,%u]",first?"":",",at,static_cast<unsigned long long>(table>image?table-image:table),
            record && record->resident?"true":"false",record?record->description.width:0u,record?record->description.height:0u,record?static_cast<unsigned>(record->description.format):0u);row+=c;first=false;}
    row+="},\"bytes\":\""+HexBytes(d.data(),d.size())+"\"";
    std::array<uintptr_t,2> queue{};uint32_t budget{};Read(manager+0x20,queue.data(),16);Read(manager+0x408,&budget,4);
    const size_t entries=queue[1]>=queue[0]?(queue[1]-queue[0])/0x30:0;
    row+=",\"queue\":"+std::to_string(entries)+",\"budget\":"+std::to_string(budget)+",\"entries\":[";
    for(size_t i=0;i<entries && i<8;++i){std::array<uint8_t,0x30> e{};Read(queue[0]+0x30*i,e.data(),e.size());row+=(i?",\"":"\"")+HexBytes(e.data(),e.size())+"\"";}
    row+="],\"receivers\":[";first=true;
    for(size_t i=0;i<entries && i<64;++i){uintptr_t receiver{},table{};Read(queue[0]+0x30*i,&receiver,8);if(!receiver || !seen.insert(receiver).second)continue;Read(receiver,&table,8);
        std::array<uintptr_t,4> slots{};uintptr_t draw{};uint32_t mask_index{},flag{};uint8_t gates[2]{};
        Read(receiver+0x20,slots.data(),32);Read(receiver+0x40,&draw,8);Read(receiver+0x58,&mask_index,4);Read(receiver+0x5C,&flag,4);Read(receiver+0x78,gates,2);
        uint8_t initialised{};std::array<uint8_t,0x20> creation{};if(slots[0]){Read(slots[0]+0x50,&initialised,1);Read(slots[0]+0x60,creation.data(),creation.size());}
        char c[256]{};sprintf_s(c,"%s{\"vt\":\"%llX\",\"slots\":[%d,%d,%d,%d],\"draw\":%s,\"mask_index\":%d,\"flag\":%u,\"gates\":[%u,%u],\"rt_init\":%u,\"rt_creation\":\"",first?"":",",
            static_cast<unsigned long long>(table>image?table-image:table),slots[0]!=0,slots[1]!=0,slots[2]!=0,slots[3]!=0,draw?"true":"false",static_cast<int>(mask_index),flag,gates[0],gates[1],initialised);
        row+=c+HexBytes(creation.data(),creation.size())+"\"}";first=false;}
    Log((row+"]}\n").c_str());}
uint32_t __fastcall RequestNativeDecalCpu(uintptr_t receiver,uintptr_t descriptor,uintptr_t callback) noexcept {
    try {
        const std::lock_guard hold(decal_cpu_lock);
        if(!decal_cpu_registries.contains(receiver))Refuse("native_decal_request_foreign_registry");
        uintptr_t world{},material{},source{};uint32_t projection{},kind{};std::array<float,3> size{};float range{},minimum_range{};
        if(!Read(receiver+8,&world,8))Refuse("native_decal_request_world_unreadable");
        // 51BF0 returns immediately when its real CPU world is absent.
        bool rejected=!world;
        if(!rejected){
            if(!descriptor || !Read(descriptor+0x24,size.data(),sizeof(size)) || !Read(descriptor+0x30,&range,4) ||
               !Read(descriptor+0x38,&kind,4) || !Read(descriptor+0x3C,&projection,4) ||
               !Read(descriptor+0x48,&material,8) || !Read(descriptor+0x78,&source,8) || !Read(image+0x3D7EE10,&minimum_range,4)){
                NoteNativeSkip("decal_request_descriptor_unreadable",descriptor);return 0;}   // I-0101: the request rejected, as 51BF0's early returns
            // Guaranteed early rejections from genuine CPU52B60. No handle,
            // generated geometry or callback is created for these requests.
            rejected=projection>1 || size[0]==0 || size[1]==0 || size[2]==0 || (!material && !source) ||
                kind>1 || (projection==0 && (range<minimum_range || std::isinf(range)));
        }
        if(rejected){
            static LONG64 count{};if(InterlockedIncrement64(&count)<=8)
                Log("{\"event\":\"native_decal_request_cpu_rejected\",\"original_early_return\":true,\"geometry_generated\":false,\"decal_draw_ready\":false}\n");
            // 51C1F/51D48 return EAX=0; the accepted path returns its real
            // generation-bearing handle at51EC4. The decompiler's void
            // prototype loses this observable engine-facing contract.
            return 0;
        }
        uintptr_t callback_table{},predicate{};
        // Firelink decal stop (ROADMAP): kind 1 (51BF0's second class, vtable 3D5B978, allocated 0x140 and initialised by
        // 19BA40, a pure CPU copy of the validated descriptor; every scene slot of its vtable is a bare RET, so it never reaches
        // a draw consumer) is owned too, through the same synchronous CPU query; kind 0 keeps its contract.
        if(!Read(callback,&callback_table,8) || callback_table!=image+0x3D20450 ||
           !Read(callback_table,&predicate,8) || predicate!=image+0x50120){
            // 1171 witness (Native1171 with Thomas's save, in game): which decal request family is not owned.
            static LONG64 family_rows{};if(const auto n=InterlockedIncrement64(&family_rows);n<=8 || n%1000==0){   // I-0101: no longer followed by a refusal
                char row[320]{};sprintf_s(row,"{\"event\":\"native_decal_request_family\",\"kind\":%u,\"projection\":%u,\"callback_table_rva\":%lld,\"predicate_rva\":%lld,\"caller\":%llu,\"material\":%s,\"source\":%s}\n",
                kind,projection,callback_table?static_cast<long long>(callback_table-image):-1ll,predicate?static_cast<long long>(predicate-image):-1ll,
                static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image),material?"true":"false",source?"true":"false");Log(row);}
            NoteNativeSkip("decal_query_family_not_owned",callback_table);return 0;}   // I-0101: that request rejected (row above), not refused
        // Retain the authentic full validation, including NaN and the CPU
        // material's projection classification, before allocating a handle.
        if(!reinterpret_cast<BYTE(__fastcall*)(uintptr_t,uintptr_t)>(image+0x52B60)(receiver,descriptor))return 0;
        NativeDecalCpuIntent intent;
        if(!Read(descriptor,intent.descriptor.data(),intent.descriptor.size())){NoteNativeSkip("decal_full_descriptor_unreadable",descriptor);return 0;}   // I-0101
        if(native_decal_query_scope)Refuse("native_decal_cpu_reentrant_request");
        NativeDecalQueryScope scope{receiver,descriptor,callback};native_decal_query_scope=&scope;
        const auto handle=decal_request_original(receiver,descriptor,callback);
        native_decal_query_scope=nullptr;
        if(!handle)return 0;
        auto& owner=decal_cpu_registries.at(receiver);
        uintptr_t object{},table{};uint16_t generation{};uint32_t actual_handle{};
        const auto index=handle&0xFFFFu;
        if(index>=owner.count || scope.completed!=1 ||
           !Read(owner.generations+uint64_t(index)*2,&generation,2) || generation!=(handle>>16) ||
           !Read(owner.buckets+uint64_t(index)*8,&object,8) || !object ||
           !Read(object,&table,8) || table!=image+(kind?0x3D5B978:0x3D5B8D8) ||
           !Read(object+0x138,&actual_handle,4) || actual_handle!=handle ||
           (!kind && !Read(object+0x140,intent.prepared.data(),intent.prepared.size())))
            Refuse("native_decal_cpu_query_result_contract");
        intent.kind=kind;
        // The true manager may evict a prior generation of the same bucket.
        // Its retained native description follows that actual slot lifetime.
        for(auto it=owner.requests.begin();it!=owner.requests.end();){
            if((it->first&0xFFFFu)==index)it=owner.requests.erase(it);else ++it;
        }
        intent.handle=handle;intent.world_layers=scope.layers;owner.requests[handle]=std::move(intent);
        static const bool observe=ModuleFlag(L"decal-observe.flag");   // DECALS-WAVE1: the request's object (kind 0), as the Reference rows
        if(observe && !kind){static LONG64 observed{};const auto n=InterlockedIncrement64(&observed);
            // the texture objects +0x190..+0x200: their vtable and whether the native texture ingress holds them resident
            std::string slots;for(uint32_t at=0x190;at<=0x200;at+=8){uintptr_t texture{},texture_table{};Read(object+at,&texture,8);if(!texture)continue;Read(texture,&texture_table,8);
                const auto record=textures.Acquire(texture);char cell[96]{};sprintf_s(cell,"%s\"%X\":[\"%llX\",%s]",slots.empty()?"":",",at,static_cast<unsigned long long>(texture_table>image?texture_table-image:texture_table),
                    record && record->resident?"true":"false");slots+=cell;}
            if(n<=200)Log(("{\"event\":\"decal_observe_request\",\"handle\":"+std::to_string(handle)+",\"textures\":{"+slots+"},\"decal\":"+DecalObjectRow(object)+"}\n").c_str());
            if(n==1)DumpNativeDecalProgrammes(owner);}
        if(observe && kind==1)ObserveNativePaintRequest(receiver,handle,owner.requests[handle].descriptor);   // DECALS-PAINT-READY rung 0
        char row[384]{};sprintf_s(row,"{\"event\":\"native_decal_cpu_request_completed\",\"handle\":%u,\"kind\":%u,\"world_layers\":%u,\"actual_cpu_object\":true,\"actual_synchronous_query\":true,\"owned_cpu_description\":true,\"legacy_gpu_work\":false,\"native_decal_draw_ready\":false}\n",handle,kind,scope.layers);Log(row);
        return handle;
    }catch(...){Refuse("native_decal_request_cpu_policy_failed");}
}
[[noreturn]] void __fastcall UnownedNativeDecalSceneConsumer() noexcept {
    Refuse("native_decal_scene_consumer_not_connected");
}
void __fastcall PrepareNativeDecalProgrammes(uintptr_t receiver,uintptr_t bank) noexcept {
    try {
        uintptr_t table{},programme_lookup{},constant_lookup{},family{};
        if(!Read(bank,&table,8) || !Read(table+0x30,&programme_lookup,8) || !programme_lookup ||
           !Read(table+0x38,&constant_lookup,8) || !constant_lookup || !Read(image+0x473C7E8,&family,8) || !family)
            Refuse("native_decal_cpu_programme_bank_unreadable");
        {const std::lock_guard lock(decal_cpu_lock);
            if(!decal_cpu_registries.contains(receiver))Refuse("native_decal_cpu_programme_foreign");}
        NativeDecalCpuRegistry selected;
        const auto lookup_programme=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(programme_lookup);
        const auto lookup_constant=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(constant_lookup);
        uint32_t programmes{},constants{};
        for(size_t index=0;index<selected.programmes.size();++index){uintptr_t name{};
            if(!Read(image+0x473C860+index*8,&name,8) || !name)Refuse("native_decal_cpu_programme_name_unreadable");
            selected.programmes[index]=lookup_programme(bank,family,name);
            if(selected.programmes[index])++programmes;
        }
        for(size_t index=0;index<selected.constant_descriptions.size();++index){uintptr_t name{};
            if(!Read(image+0x473C7F0+index*8,&name,8) || !name)Refuse("native_decal_cpu_constant_name_unreadable");
            selected.constant_descriptions[index]=lookup_constant(bank,family,name);
            if(selected.constant_descriptions[index])++constants;
        }
        selected.compute_programme=lookup_programme(bank,image+0x3D410F8,image+0x3D410D8);
        // Actual native86 executed these same CPU lookups before51320's first
        // excluded old-device access. Preserve their authentic CPU results;
        // no invocation of old CB factories1925210/19250A0 or fake buffer.
        const std::lock_guard lock(decal_cpu_lock);
        auto& owned=decal_cpu_registries.at(receiver);
        owned.programmes=selected.programmes;owned.constant_descriptions=selected.constant_descriptions;
        owned.compute_programme=selected.compute_programme;
        {static const bool observe=ModuleFlag(L"decal-observe.flag");static bool dumped{};if(observe && !dumped){dumped=true;DumpNativeDecalProgrammes(owned);}}
        std::memcpy(reinterpret_cast<void*>(receiver+0x98),owned.programmes.data(),sizeof(owned.programmes));
        std::memcpy(reinterpret_cast<void*>(receiver+0x210),owned.constant_descriptions.data(),sizeof(owned.constant_descriptions));
        *reinterpret_cast<uintptr_t*>(receiver+0x90)=owned.compute_programme;
        char row[256]{};sprintf_s(row,"{\"event\":\"native_decal_cpu_programmes_selected\",\"programmes\":%u,\"constant_descriptions\":%u,\"compute_present\":%s,\"legacy_gpu_buffers\":false,\"native_decal_draw_ready\":false}\n",
            programmes,constants,owned.compute_programme?"true":"false");Log(row);
    }catch(...){Refuse("native_decal_cpu_programme_preparation_failed");}
}
uintptr_t __fastcall ConstructNativeSceneTreeCpu(uintptr_t receiver,uintptr_t heap,uint64_t arena_bytes) noexcept {
    try {
        // RTTI534 proves a CPU GXSgNumTreeRootNode, not a GPU command queue.
        // Preserve its actual spatial tree, arena and heap-wrapper algorithms.
        // The loaded AE4A0/AE670 pair only records named heaps in the excluded
        // renderer's diagnostic registry. Native owns that registration here.
        if(!receiver || !heap)Refuse("native_scene_tree_cpu_constructor_input");
        if(!arena_bytes)arena_bytes=0x80000;
        if(arena_bytes>(64ull<<20))Refuse("native_scene_tree_cpu_arena_bound");
        const auto base=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t)>(image+0x207250)(receiver,heap,0,0);
        if(base!=receiver)Refuse("native_scene_tree_cpu_base_constructor");
        *reinterpret_cast<uintptr_t*>(receiver)=image+0x3D619D8;
        *reinterpret_cast<uintptr_t*>(receiver+0x100)=0;
        if(reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0xE2A0)(receiver+0x108)!=receiver+0x108)
            Refuse("native_scene_tree_cpu_arena_constructor");
        *reinterpret_cast<uintptr_t*>(receiver+0x5B0)=0;
        *reinterpret_cast<uint8_t*>(receiver+0x5B8)=1;
        *reinterpret_cast<uintptr_t*>(receiver+0x5C0)=0;
        if(!reinterpret_cast<uint8_t(__fastcall*)(uintptr_t,uintptr_t,uint64_t,uint32_t)>(image+0xE350)(receiver+0x108,heap,arena_bytes,0))
            Refuse("native_scene_tree_cpu_arena_initialization");
        const auto wrapper=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0x18,8,heap);
        if(!wrapper)Refuse("native_scene_tree_cpu_heap_allocation");
        *reinterpret_cast<uintptr_t*>(wrapper)=image+0x3D4AD68;
        *reinterpret_cast<uintptr_t*>(wrapper+8)=receiver+0x120;
        *reinterpret_cast<uintptr_t*>(wrapper+0x10)=heap;
        *reinterpret_cast<uintptr_t*>(receiver+0x5B0)=wrapper;
        *reinterpret_cast<uintptr_t*>(receiver+8)=wrapper;
        std::array<uint32_t,4> mask{},homogeneous{},minimum{},maximum{};
        if(!Read(image+0x39F2650,mask.data(),16) || !Read(image+0x3D1C120,homogeneous.data(),16) ||
           !Read(image+0x3D80020,minimum.data(),16) || !Read(image+0x3D7FF70,maximum.data(),16))
            Refuse("native_scene_tree_cpu_bounds_source");
        for(size_t component=0;component<4;++component){
            minimum[component]=(~mask[component]&minimum[component])|(mask[component]&homogeneous[component]);
            maximum[component]=(~mask[component]&maximum[component])|(mask[component]&homogeneous[component]);}
        for(const uintptr_t offset:{0x60,0x80,0xA0}){
            std::memcpy(reinterpret_cast<void*>(receiver+offset),minimum.data(),16);
            std::memcpy(reinterpret_cast<void*>(receiver+offset+0x10),maximum.data(),16);}
        {std::lock_guard<std::mutex> hold(scene_tree_cpu_lock);
            if(!scene_tree_cpu_owners.emplace(receiver,NativeSceneTreeCpu{heap,wrapper,arena_bytes}).second)
                Refuse("native_scene_tree_cpu_duplicate_owner");}
        char row[288]{};sprintf_s(row,"{\"event\":\"native_scene_tree_cpu_heap_registered\",\"family\":\"GXSgNumTreeRootNode\",\"arena_bytes\":%llu,\"actual_cpu_spatial_tree\":true,\"legacy_renderer_registry\":false,\"scene_rendered\":false}\n",arena_bytes);Log(row);
        return receiver;
    }catch(...){Refuse("native_scene_tree_cpu_construction_failed");}
}
void __fastcall RetireNativeSceneTreeCpu(uintptr_t receiver) noexcept {
    try {
        NativeSceneTreeCpu owned;
        {std::lock_guard<std::mutex> hold(scene_tree_cpu_lock);const auto found=scene_tree_cpu_owners.find(receiver);
            if(found==scene_tree_cpu_owners.end())Refuse("native_scene_tree_cpu_foreign_retirement");owned=found->second;}
        uintptr_t wrapper{},context{},debug_tree{};
        if(!Read(receiver+0x5B0,&wrapper,8) || wrapper!=owned.heap_wrapper ||
           !Read(receiver+0xF0,&context,8) || context || !Read(receiver+0x5C0,&debug_tree,8) || debug_tree)
            Refuse("native_scene_tree_cpu_retirement_contract");
        *reinterpret_cast<uintptr_t*>(receiver)=image+0x3D619D8;
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x20A360)(receiver);
        // Native withdraws its named CPU heap registration. No access to the
        // old renderer+ B58 registry occurs in construction or retirement.
        {std::lock_guard<std::mutex> hold(scene_tree_cpu_lock);scene_tree_cpu_owners.erase(receiver);}
        reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uint8_t)>(image+0xD8200)(wrapper,0);
        FreeModelPrefix(wrapper,owned.backing_heap);
        *reinterpret_cast<uintptr_t*>(receiver+0x5B0)=0;
        reinterpret_cast<uint8_t(__fastcall*)(uintptr_t)>(image+0xE400)(receiver+0x108);
        if(reinterpret_cast<uint8_t(__fastcall*)(uintptr_t)>(image+0x17F2750)(receiver+0x130))
            reinterpret_cast<uint8_t(__fastcall*)(uintptr_t)>(image+0xE400)(receiver+0x108);
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x1799370)(receiver+0x570);
        *reinterpret_cast<uintptr_t*>(receiver+0x120)=image+0x279B458;
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x207470)(receiver);
        Log("{\"event\":\"native_scene_tree_cpu_heap_retired\",\"legacy_renderer_registry\":false}\n");
    }catch(...){Refuse("native_scene_tree_cpu_retirement_failed");}
}
[[noreturn]] void __fastcall UnsupportedSceneContext() noexcept {
    Refuse("native_scene_context_method_not_owned");
}
uintptr_t __fastcall RetireNativeSceneContext(uintptr_t receiver,uint32_t flags) noexcept {
    try {
        const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
        const auto found=scene_context_cpu_owners.find(receiver);
        if(found==scene_context_cpu_owners.end())Refuse("native_scene_context_retire_foreign");
        const auto owned=found->second;
        if(active_scene_context==receiver){active_scene_context=0;native_scene_camera.reset();native_scene_camera_bytes.reset();native_scene_camera_constants.reset();ResetNativeSelectedScene();}
        if(owned.secondary){
            RetireNativeSceneContext(owned.secondary,0);FreeModelPrefix(owned.secondary,owned.heap);
            *reinterpret_cast<uintptr_t*>(receiver+0x1D0)=0;
        }
        scene_context_cpu_owners.erase(receiver);
        if(flags&1u)FreeModelPrefix(receiver,owned.heap);
        Log("{\"event\":\"native_scene_context_cpu_retired\",\"legacy_gpu_object\":false}\n");
        return receiver;
    }catch(...){Refuse("native_scene_context_retire_failed");}
}
const std::array<uintptr_t,3>& NativeSceneContextTable() {
    static const auto table=[](){std::array<uintptr_t,3> result{};
        result.fill(reinterpret_cast<uintptr_t>(&UnsupportedSceneContext));
        result[0]=reinterpret_cast<uintptr_t>(&RetireNativeSceneContext);return result;}();
    return table;
}
uintptr_t NewNativeSceneContext(uintptr_t heap,const std::array<BYTE,0x26>& features,bool secondary) {
    // StageRend writes through context+415C and child+6C. Keep that CPU ABI,
    // with a native vtable and explicit semantic fields, rather than claiming
    // that a zeroed original GX render context has completed initialization.
    const auto context=reinterpret_cast<uintptr_t(__fastcall*)(uint64_t,uint64_t,uintptr_t)>(image+0x1777A00)(0x4170,16,heap);
    if(!context)throw std::bad_alloc();
    std::memset(reinterpret_cast<void*>(context),0,0x4170);
    *reinterpret_cast<uintptr_t*>(context)=reinterpret_cast<uintptr_t>(NativeSceneContextTable().data());
    *reinterpret_cast<uintptr_t*>(context+0x18)=heap;
    *reinterpret_cast<uint32_t*>(context+0x20)=output_width;
    *reinterpret_cast<uint32_t*>(context+0x24)=output_height;
    *reinterpret_cast<uint32_t*>(context+0x28)=1024;
    *reinterpret_cast<uint32_t*>(context+0x2C)=1024;
    uint64_t policy{};std::memcpy(&policy,features.data()+8,8);
    *reinterpret_cast<uint64_t*>(context+0x30)=(policy&(uint64_t{1}<<21))?0x400000004ull:1;
    *reinterpret_cast<uint64_t*>(context+0x50)=policy;
    *reinterpret_cast<uint16_t*>(context+0x58)=0x101;
    *reinterpret_cast<uint16_t*>(context+0x5B)=0x101;
    constexpr size_t feature_offsets[]{0x5A,0x5D,0x5E,0x5F,0x60,0x61,0x62,0x63};
    for(size_t offset=0;offset<8;++offset)*reinterpret_cast<BYTE*>(context+feature_offsets[offset])=features[0x1E+offset];
    *reinterpret_cast<uint32_t*>(context+0x6C)=0xFF003020;
    // Genuine CPU matrix/lens defaults. Actual native presentation dimensions
    // replace the absent old swap-chain source, without synthesizing a camera.
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x04B5C0)(context+0x3EE0);
    *reinterpret_cast<uint32_t*>(context+0x3F3C)=output_width;
    *reinterpret_cast<uint32_t*>(context+0x3F40)=output_height;
    // Actual CPU transition defaults from 0843C0. These are authored selection
    // pairs, not GPU handles; zero is not the original "no target" sentinel.
    constexpr size_t transition_pairs[]{0x3FA8,0x3FB0,0x3FC0,0x3FC8,
        0x40E0,0x40E8,0x40F8,0x4100,0x4110,0x4118,0x4128,0x4130};
    for(const auto offset:transition_pairs)*reinterpret_cast<uint32_t*>(context+offset+4)=0xFFFFFFFF;
    for(size_t index=0;index<13;++index){
        *reinterpret_cast<uint32_t*>(context+0x3FDC+index*8)=0xFFFFFFFF;
        *reinterpret_cast<uint32_t*>(context+0x4044+index*8)=0xFFFFFFFF;
    }
    // W5a SW-7: 0843C0's shadow flag bytes (the qword 0x01010001 at +0x3EBC): +0x3EBC the cascaded-shadow gate, +0x3EBE, and
    // +0x3EBF the local-light shadow gate of 10A220 (1400885F0 later copies +0x3EBE into +0x3EBF). Row SL1 (Reference1507)
    // reads 1 at +0x3EBC and +0x3EBF in game. The neighbouring 843C0 flags (+0x3EB0, +0x3EB8..+0x3EBB) stay open.
    *reinterpret_cast<uint64_t*>(context+0x3EBC)=0x01010001;
    const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
    scene_context_cpu_owners.emplace(context,NativeSceneContextCpu{heap,0});
    if(!secondary){
        const auto child=NewNativeSceneContext(heap,features,true);
        scene_context_cpu_owners.at(context).secondary=child;
        *reinterpret_cast<uintptr_t*>(context+0x1D0)=child;
    }
    return context;
}
[[noreturn]] void __fastcall UnsupportedSceneView() noexcept {
    Refuse("native_scene_view_submission_not_owned");
}
void __fastcall SetNativeSceneContext(uintptr_t receiver,uintptr_t context) noexcept {
    const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
    const auto found=scene_view_cpu_owners.find(receiver);
    if(found==scene_view_cpu_owners.end() || context!=found->second.context)
        Refuse("native_scene_view_context_replacement_not_owned");
}
uintptr_t __fastcall RetireNativeSceneView(uintptr_t receiver,uint32_t flags) noexcept {
    try {
        const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
        const auto found=scene_view_cpu_owners.find(receiver);
        if(found==scene_view_cpu_owners.end())Refuse("native_scene_view_retire_foreign");
        const auto owned=found->second;
        // Actual CPU base retirement releases its world, camera, vector and
        // our context through their real heap contracts. No old context pool.
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x204480)(receiver);
        scene_view_cpu_owners.erase(receiver);
        if(flags&1u)FreeModelPrefix(receiver,owned.heap);
        Log("{\"event\":\"native_scene_view_cpu_retired\",\"legacy_context_pool\":false}\n");
        return receiver;
    }catch(...){Refuse("native_scene_view_retire_failed");}
}
const std::array<uintptr_t,14>& NativeSceneViewTable() {
    static const auto table=[](){std::array<uintptr_t,14> result{};
        result.fill(reinterpret_cast<uintptr_t>(&UnsupportedSceneView));
        result[0]=reinterpret_cast<uintptr_t>(&RetireNativeSceneView);
        result[1]=image+0x2045B0;result[2]=image+0x204620;result[3]=image+0x204630;
        result[4]=reinterpret_cast<uintptr_t>(&SetNativeSceneContext);
        result[5]=image+0x204650;result[6]=image+0x2046A0;
        result[8]=image+0xC2C10; // Proven original empty notification.
        return result;}();
    return table;
}
uintptr_t __fastcall ConstructNativeSceneView(uintptr_t,uintptr_t feature_source) noexcept {
    try {
        if(!opened || !native_resource_consumers || !output_width || !output_height)
            Refuse("native_scene_view_without_native_presentation");
        std::array<BYTE,0x26> features{};uintptr_t heap{};
        if(!Read(feature_source,features.data(),features.size()) || !Read(image+0x4961B80,&heap,8) || !heap)
            Refuse("native_scene_view_cpu_input_unreadable");
        const auto context=NewNativeSceneContext(heap,features,false);
        // C3F20 is genuine CPU world/refcount/decal construction. Its mixed
        // decal initializer is already replaced by the owned CPU registry.
        const auto world=reinterpret_cast<uintptr_t(__fastcall*)()>(image+0xC3F20)();
        const auto view=reinterpret_cast<uintptr_t(__fastcall*)(uint64_t,uint64_t,uintptr_t)>(image+0x1777A00)(0x100,16,heap);
        if(!view || !world)throw std::bad_alloc();
        reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(image+0x2043A0)(view,world,context);
        *reinterpret_cast<uintptr_t*>(view)=reinterpret_cast<uintptr_t>(NativeSceneViewTable().data());
        *reinterpret_cast<uintptr_t*>(view+0xF0)=0;*reinterpret_cast<uintptr_t*>(view+0xF8)=0;
        // Disassembly proves the base camera input starts at50 (RCX at
        // 204424), despite the decompiler omitting this helper argument.
        *reinterpret_cast<uint32_t*>(view+0xAC)=output_width;
        *reinterpret_cast<uint32_t*>(view+0xB0)=output_height;
        {const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
            scene_view_cpu_owners.emplace(view,NativeSceneViewCpu{heap,context,++scene_view_generation,features});}
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0x2045B0)(view,world);
        Log("{\"event\":\"native_scene_view_cpu_created\",\"genuine_cpu_world\":true,\"native_camera_and_clear_intent\":true,\"legacy_gpu_objects\":false,\"scene_submission_ready\":false}\n");
        return view;
    }catch(...){Refuse("native_scene_view_create_failed");}
}
void __fastcall PublishNativeSceneContext(uintptr_t context) noexcept {
    const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
    // 1262 (Native1261, quit in game): 17E30 stores its argument as the active render context ([144961BF8]); the area
    // teardown (DFF7C0 ...) stores nil. Natively nil withdraws the active context, its camera and its selection.
    if(!context){active_scene_context=0;native_scene_camera.reset();native_scene_camera_bytes.reset();native_scene_camera_constants.reset();ResetNativeSelectedScene();
        static LONG64 withdrawn{};if(InterlockedIncrement64(&withdrawn)<=4)Log("{\"event\":\"native_scene_context_withdrawn\"}\n");return;}
    const auto found=scene_context_cpu_owners.find(context);
    if(found==scene_context_cpu_owners.end() || !found->second.secondary)
        {char row[200]{};sprintf_s(row,"{\"event\":\"native_scene_context_foreign\",\"known\":%s,\"caller\":%llu}\n",found!=scene_context_cpu_owners.end()?"true":"false",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);
        Refuse("native_scene_context_publication_foreign");}
    active_scene_context=context;native_scene_camera.reset();native_scene_camera_bytes.reset();native_scene_camera_constants.reset();ResetNativeSelectedScene();
    Log("{\"event\":\"native_scene_context_published\",\"legacy_renderer_global\":false}\n");
}
// I-0105 tranche 10 (GX-DEPENDENCIES C6: the root members D1E8A0 writes, left out as unneeded): 103510 binds the root's
// +0x4800 (albedo), +0x4808 (normal), +0x4810, +0x4818 as the snow textures t47, t50, t48, t49. D59E90 stores there the
// repository holders ([entry+0x60]) of global_snow_a, _n, _r, _s once its resources are loaded, D1E8A0 those of snow_a, _n, _r, _s
// (D1EA10, dust_a.., runs genuinely natively); a key not found leaves its member. `keys`: the four keys in that order.
constexpr uint32_t native_surface_members[4]{0x4800,0x4808,0x4810,0x4818};
// Witness (both modes, final review M1/L6): the root's members +0x4800..+0x4820 after one of their writers ran, on each change
// (bounded), so the objects 103510 and the native bind are named by their writer (rows native_surface_store).
void LogNativeSurfaceStore(const char* writer) noexcept {
    uintptr_t root{};std::array<uintptr_t,5> members{};
    if(!Read(image+0x4961B50,&root,8) || !root || !Read(root+0x4800,members.data(),sizeof(members)))return;
    static std::mutex lock;static std::array<uintptr_t,5> last{~uintptr_t{}};static int rows{};   // a sentinel: an all-null root is logged too (step review L-b)
    {const std::lock_guard hold(lock);if(members==last || rows>=32)return;last=members;++rows;}
    char row[360]{};sprintf_s(row,"{\"event\":\"native_surface_store\",\"mode\":\"%s\",\"after\":\"%s\",\"members_4800_4808_4810_4818_4820\":[\"%llX\",\"%llX\",\"%llX\",\"%llX\",\"%llX\"]}\n",
        control->mode==bootstrap::Mode::Reference?"reference":"native",writer,static_cast<unsigned long long>(members[0]),static_cast<unsigned long long>(members[1]),
        static_cast<unsigned long long>(members[2]),static_cast<unsigned long long>(members[3]),static_cast<unsigned long long>(members[4]));Log(row);
}
// Reference with ffx-draw-observe.flag: D1EA10 (dust_a, _n, _r, _s into the same members; genuine in both modes) and D1E8A0 (natively replaced below).
using NativeSurfaceWriter=void(__fastcall*)();NativeSurfaceWriter dust_store_original{},snow_store_original{};
void __fastcall DustStoreObserved() noexcept {dust_store_original();LogNativeSurfaceStore("D1EA10");}
void __fastcall SnowStoreObserved() noexcept {snow_store_original();LogNativeSurfaceStore("D1E8A0");}
uintptr_t NativeSurfaceRoot() {uintptr_t root{};if(!Read(image+0x4961B50,&root,8) || !root)Refuse("native_surface_textures_root_missing");return root;}
void StoreNativeSurfaceTextures(uintptr_t repository,const std::array<uintptr_t,4>& keys) {
    const auto root=NativeSurfaceRoot();
    for(size_t index=0;index<keys.size();++index){
        const auto entry=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0xD4ED20)(repository,keys[index]);
        uintptr_t holder{};if(entry && Read(entry+0x60,&holder,8))*reinterpret_cast<uintptr_t*>(root+native_surface_members[index])=holder;}
    LogNativeSurfaceStore("D59E90");
}
void __fastcall PublishNativeStageTextures() noexcept {
    try {
        uintptr_t repository{};
        if(!Read(image+0x4792F58,&repository,8) || !repository)
            Refuse("native_stage_texture_repository_missing");
        const auto root=NativeSurfaceRoot();
        std::array<std::shared_ptr<const bootstrap::NativeTextureRecord>,4> selected;
        {const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);selected=native_stage_textures;}
        uint32_t admitted{};
        for(size_t index=0;index<selected.size();++index){
            // D1E8A0's four exact authored resource keys. D4ED20 is the
            // genuine CPU repository lookup, not a graphics call.
            const auto entry=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0xD4ED20)
                (repository,image+0x294DA80+index*16);
            if(!entry)continue; // Original semantics retain the previous key.
            uintptr_t holder{},holder_table{},wrapper{};
            // Same source-qualified CPU holder as NativeUiTextureImport:
            // the repository's +60 is the holder, whose +10 is the texture.
            if(!Read(entry+0x60,&holder,8) || !holder || !Read(holder,&holder_table,8) ||
               holder_table!=image+0x29559E8 || !Read(holder+0x10,&wrapper,8) || !wrapper)
                Refuse("native_stage_texture_cpu_entry_unreadable");
            selected[index]=textures.Acquire(wrapper);
            if(!selected[index] || !selected[index]->resident || !selected[index]->content ||
               !selected[index]->resource || !selected[index]->upload_fence){
                char row[256]{};sprintf_s(row,"{\"event\":\"native_stage_texture_unowned\",\"slot\":%zu,\"wrapper\":%llu}\n",
                    index,static_cast<unsigned long long>(wrapper));Log(row);
                Refuse("native_stage_texture_residency_missing");
            }
            *reinterpret_cast<uintptr_t*>(root+native_surface_members[index])=holder;   // D1E8A0's root store (as StoreNativeSurfaceTextures does D59E90's)
            ++admitted;
        }
        LogNativeSurfaceStore("D1E8A0");
        const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
        native_stage_textures=std::move(selected);++native_stage_texture_generation;
        char row[256]{};sprintf_s(row,"{\"event\":\"native_stage_textures_published\",\"resident_textures\":%u,\"generation\":%llu,\"legacy_renderer_slots\":false,\"scene_draw_ready\":false}\n",
            admitted,native_stage_texture_generation);Log(row);
    }catch(...){Refuse("native_stage_texture_publication_failed");}
}
void __fastcall PublishNativePostTransitionIntent(uintptr_t input) noexcept {
    try {
        uintptr_t row{};NativePostTransitionIntent selected;
        if(!Read(input,&row,8))Refuse("native_post_transition_input_unreadable");
        if(row){std::array<BYTE,0x2A> values{};
            if(!Read(row,values.data(),values.size())){NoteNativeSkip("post_transition_row_unreadable",row);return;}   // I-0101: skipped (nothing published, the count unchanged)
            std::memcpy(selected.controls.data(),values.data(),8);
            std::memcpy(selected.intervals.data(),values.data()+8,16);
            for(size_t index=0;index<selected.targets.size();++index)
                selected.targets[index]=values[0x18+index]>20?20:values[0x18+index];
        }else{
            if(!Read(image+0x2988808,selected.controls.data(),8))Refuse("native_post_transition_defaults_unreadable");
            selected.intervals={5,20,30,10};selected.targets.fill(20);
        }
        for(const auto value:selected.controls)if(!std::isfinite(value)){NoteNativeSkip("post_transition_control_invalid",row);return;}   // I-0101
        // Source570 is a complete CPU publication, not a GPU execution route.
        // Param-table85/row2's six controls and eighteen bounded targets are read and checked, then counted and logged
        // (I-0137, I-0120 A8: no native consumer reads them, so they are no longer stored). The old higher-renderer+5368 object is never made
        // or populated. Post processing still requires its native consumer.
        selected.generation=native_post_transition_generation.fetch_add(1,std::memory_order_relaxed)+1;
        char log[256]{};sprintf_s(log,"{\"event\":\"native_post_transition_intent\",\"authored_row\":%s,\"generation\":%llu,\"targets\":18,\"legacy_renderer_state\":false,\"native_post_draw_ready\":false}\n",
            row?"true":"false",selected.generation);Log(log);
    }catch(...){Refuse("native_post_transition_publication_failed");}
}
std::atomic<uintptr_t> native_scene_camera_object{};   // FFX draw N5a: the GX camera record CED670 fills for the main view (E0081C)
void __fastcall ProduceNativeSceneCamera(void* receiver,void* output) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());
    scene_camera_original(receiver,output);
    if(caller!=image+0xE0081C)return;
    uintptr_t table{};
    if(!Read(reinterpret_cast<uintptr_t>(receiver),&table,8) || table!=image+0x27D9F08)
        Refuse("native_scene_camera_cpu_identity");
    // CED670's actual CPU lens is tied to our current native viewport.
    const auto destination=reinterpret_cast<uintptr_t>(output);
    *reinterpret_cast<uint32_t*>(destination+0x5C)=output_width;
    native_scene_camera_object.store(destination,std::memory_order_release);
    *reinterpret_cast<uint32_t*>(destination+0x60)=output_height;
    SceneViewInput camera;
    if(!ingress::CopySceneView(reinterpret_cast<uintptr_t>(receiver),destination,caller,image,serial+1,camera))
        Refuse("native_scene_camera_cpu_input_unsupported");
    const std::lock_guard<std::recursive_mutex> lock(scene_view_cpu_lock);
    if(!active_scene_context)return;
    ApplyUpscalerJitter(camera);   // I-0111
    ingress::SceneCameraConstants constants;
    if(!ingress::ProduceCameraConstants(camera,constants))Refuse("native_scene_camera_existing_producer_failed");
    native_scene_camera=camera;native_scene_camera_constants=std::move(constants);
    {std::array<uint8_t,0x88> bytes{};if(Read(destination,bytes.data(),bytes.size()))native_scene_camera_bytes=bytes;else native_scene_camera_bytes.reset();}
    static LONG64 samples{};if(InterlockedIncrement64(&samples)<=4)
        Log("{\"event\":\"native_scene_camera_cpu_produced\",\"genuine_current_pose\":true,\"native_viewport\":true,\"scene_draw_submitted\":false}\n");
}
void UpdateNativeSceneAuxiliaryView(uintptr_t service,uintptr_t camera_output) {
    NativeSceneAuxiliaryView owned;owned.width=output_width;owned.height=output_height;
    alignas(16) std::array<float,16> projection{};
    const auto produced=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,void*)>(image+0x04B4C0)
        (camera_output,projection.data());
    if(!Read(produced,owned.projection.data(),sizeof(owned.projection)) ||
       !Read(camera_output+0x30,owned.position.data(),sizeof(owned.position)))
        Refuse("native_scene_auxiliary_view_unreadable");
    // Loaded A21490 publishes the projection/position then overwrites its
    // requested dimensions from the old device. Native owns those dimensions.
    *reinterpret_cast<float*>(service+8)=static_cast<float>(owned.width);
    *reinterpret_cast<float*>(service+12)=static_cast<float>(owned.height);
    std::memcpy(reinterpret_cast<void*>(service+0x10),owned.projection.data(),sizeof(owned.projection));
    std::memcpy(reinterpret_cast<void*>(service+0x50),owned.position.data(),sizeof(owned.position));
    // Loaded A21510's disabled branch is a genuine early return. Enabled
    // selection has additional scene/UI consumers and is not forwarded blind.
    BYTE enabled{};if(!Read(service+0xD8,&enabled,1))Refuse("native_scene_auxiliary_selection_unreadable");
    if(enabled)Refuse("native_scene_auxiliary_selection_not_owned");
}
void UpdateNativeSceneDebugRequests(uintptr_t service) {
    std::array<uintptr_t,2> range{};
    if(!Read(service,range.data(),sizeof(range)) || range[1]<range[0] ||
       (range[1]-range[0])%8 || (range[1]-range[0])/8>4096)
        Refuse("native_scene_debug_request_extent");
    // D093D0/D0A770 traverse actual CPU requests. Loaded D0A510 reaches
    // old renderer+42B0 only for enabled diagnostic drawings. Preserve the
    // proven inactive branch; active drawings require a native encoder.
    for(auto at=range[0];at!=range[1];at+=8){
        uintptr_t group{};std::array<uintptr_t,2> requests{};
        if(!Read(at,&group,8) || !group || !Read(group+8,requests.data(),sizeof(requests)) ||
           requests[1]<requests[0] || (requests[1]-requests[0])%0x20 ||
           (requests[1]-requests[0])/0x20>4096)
            Refuse("native_scene_debug_group_extent");
        for(auto request=requests[0];request!=requests[1];request+=0x20){
            std::array<BYTE,2> flags{};
            // I-0101: a debug draw request (never seen in play) is not drawn natively, counted, instead of closing the game.
            if(!Read(request+0x18,flags.data(),flags.size())){NoteNativeSkip("scene_debug_flags_unreadable",request);continue;}
            if(flags[0] || flags[1])NoteNativeSkip("scene_debug_draw_not_owned",request);
        }
    }
}
// Native ownership of the genuine model-matrix CPU prefix (15420/15490).
// The old GPU arrays stay nil; current rows are direct inputs to the native
// producers, with immutable copies retained independently of engine withdrawal.
// FrpgFgTextureIns retains its real CPU image/producer. Native texture
// residency is external to its old render-target and texture-set slots.
struct NativeFaceImage {
    uintptr_t input{},model_materials{};
    std::shared_ptr<const bootstrap::NativeTextureRecord> current,composed;
};
std::shared_mutex face_image_lock;   // I-0097 tranche 2: shared for the collect's lookups (NativeComposedFace), exclusive for the owners' changes
// I-0097 E5 step review: the owners' hooks (initialise, update on the FaceGen worker, compose on the main thread, retire, close)
// in turn, as the face lock alone made them before; the collect's lookups never take it.
std::mutex face_owner_lock;
std::unordered_map<uintptr_t,NativeFaceImage> face_images;
bool face_images_closed{};
// I-0097 E5 final review: a composition never waits for an owner hook (the main thread's C3F980 every frame; C36FC0's cadence
// is not documented) and is never lost: while face_owner_lock is held it is queued, and whoever holds the lock composes the
// queued faces before releasing it (FaceOwnerTurn); a face retired or closed meanwhile is skipped.
std::mutex face_compose_queue_lock;std::vector<uintptr_t> face_compose_queue;
uintptr_t NativeHolderInner(uintptr_t holder,bool* valid=nullptr) noexcept;   // 08_model_programmes.inc
void ComposeNativeFaceLocked(uintptr_t receiver,bool queued) {   // under face_owner_lock
    std::shared_ptr<const bootstrap::NativeTextureRecord> current,previous;   // I-0097 E5: read under the lock, composed without it
    {const std::shared_lock hold(face_image_lock);const auto found=face_images.find(receiver);
        if(found==face_images.end() || face_images_closed){if(queued)return;throw std::runtime_error("native FaceGen composition owner");}
        current=found->second.current;previous=found->second.composed;}
    uint32_t status{};if(!Read(receiver+0x70,&status,4))throw std::runtime_error("native FaceGen composition status");
    if(status!=2)return;
    *reinterpret_cast<uint32_t*>(receiver+0x70)=3;
    if(!current)return; // Actual: no ready image keeps status 3.
    uintptr_t mark{};std::array<float,8> layer{};
    if(!Read(receiver+0x40,&mark,8) || !Read(receiver+0x48,layer.data(),16) || !Read(receiver+0x60,layer.data()+4,16))
        throw std::runtime_error("native FaceGen mark layer");
    // I-0113 tranche 16 (STUDY-NOT-YET-NATIVE row 16): the mark layer. 836E40 places the quad 142330050 records from the centre
    // (+0x48 x 2048, +0x4C x 1024), the angle +0x50 (radians) and the scale +0x54: corners C -+ (1024 s cos, 512 s sin) -+
    // (-1024 s sin, 512 s cos); 142377FA0 maps them by the drawer view's size (+0x110/+0x114), its matrix the identity
    // (+0x210 null): x' = 2x / W - 1, y' = 1 - 2y / H, into the face target's 512x512 viewport. The view's size was the
    // window's at 1280x720 (reference_quad_replay, i113-t16-c-original, with A and B as computed here to the digit); taken
    // as the output size, so the mark's place depends on it (INFERRED: no other size witnessed). The texture is a
    // SprjGxTexture holder (vtable 29559E8, reference_face_mark); without a resident record (or an output size yet) the face
    // is composed without its mark, counted as a skip (no stand-in, D-0197; open: the original then draws the colour alone,
    // 1423781F0, not reproduced).
    bootstrap::TextureIngress::FaceMarkLayer layer_input;const char* mark_policy="absent";
    if(mark){
        auto record=textures.Acquire(NativeHolderInner(mark));const float W=float(output_width),H=float(output_height);
        if(!record || !record->resource || !record->view){mark_policy="texture_not_resident";NoteNativeSkip("face_mark_texture_not_resident",receiver);}   // Acquire: resident only
        else if(record->description.cube || record->description.slices!=1){mark_policy="texture_unusable";NoteNativeSkip("face_mark_texture_unusable",receiver);}
        else if(W<1.f || H<1.f){mark_policy="no_output_size";NoteNativeSkip("face_mark_no_output_size",receiver);}
        else{const float s=layer[3],sine=std::sin(layer[2]),cosine=std::cos(layer[2]);const float cx=layer[0]*2048.f,cy=layer[1]*1024.f;
            const float ux=1024.f*s*cosine,uy=512.f*s*sine,vx=-1024.f*s*sine,vy=512.f*s*cosine;
            const std::array<std::array<float,2>,4> corners{{{cx-ux-vx,cy-uy-vy},{cx+ux-vx,cy+uy-vy},{cx-ux+vx,cy-uy+vy},{cx+ux+vx,cy+uy+vy}}};
            for(size_t i=0;i<4;++i){layer_input.corners[2*i]=2.f*corners[i][0]/W-1.f;layer_input.corners[2*i+1]=1.f-2.f*corners[i][1]/H;}
            std::copy(layer.begin()+4,layer.end(),layer_input.colour.begin());layer_input.texture=std::move(record);mark_policy="drawn";}}
    auto composed=textures.AdmitComposedFace(receiver+0x28,*current,0x200,0x200,layer_input.texture?&layer_input:nullptr);
    {const std::lock_guard hold(face_image_lock);const auto found=face_images.find(receiver);
        if(found==face_images.end() || face_images_closed)throw std::runtime_error("native FaceGen composition owner");
        found->second.composed=composed;}
    // Step review: the previous composed face was withdrawn when this one was installed, before it was published here; a
    // record that took it in between is told again now, so it takes this one (a first composition replaced nothing).
    if(previous)textures.NoteRepublished();
    *reinterpret_cast<uint32_t*>(receiver+0x70)=0;*reinterpret_cast<BYTE*>(receiver+0x91)=1;   // after the publication
    // The mark's clip-space corners as drawn (reference_quad_replay's A and B through the view's size).
    char row[640]{};sprintf_s(row,
        "{\"event\":\"native_face_composed\",\"generation\":%llu,\"base_generation\":%llu,\"width\":512,\"height\":512,\"mark_layer\":%s,\"mark\":[%g,%g,%g,%g],\"mark_colour\":[%g,%g,%g,%g],\"mark_policy\":\"%s\",\"queued\":%s,"
        "\"mark_corners\":[%g,%g,%g,%g,%g,%g,%g,%g],\"extent\":[%u,%u]}\n",
        composed->generation,current->generation,mark?"true":"false",
        layer[0],layer[1],layer[2],layer[3],layer[4],layer[5],layer[6],layer[7],mark_policy,queued?"true":"false",
        layer_input.corners[0],layer_input.corners[1],layer_input.corners[2],layer_input.corners[3],layer_input.corners[4],layer_input.corners[5],
        layer_input.corners[6],layer_input.corners[7],output_width,output_height);Log(row);
}
// A turn of face_owner_lock: on release, the compositions queued meanwhile are made, then the lock is let go; one queued after
// that is made by its own caller, which tries the lock again once it has queued it.
struct FaceOwnerTurn {
    mutable std::unique_lock<std::mutex> owner;   // released by the destructor (a const turn too)
    explicit FaceOwnerTurn(bool wait){if(wait)owner=std::unique_lock(face_owner_lock);else owner=std::unique_lock(face_owner_lock,std::try_to_lock);}
    ~FaceOwnerTurn(){if(!owner.owns_lock())return;
        for(;;){std::vector<uintptr_t> queued;{const std::lock_guard hold(face_compose_queue_lock);queued.swap(face_compose_queue);}
            for(const auto receiver:queued)try{ComposeNativeFaceLocked(receiver,true);}catch(...){Refuse("native_face_composition_failed");}
            owner.unlock();
            {const std::lock_guard hold(face_compose_queue_lock);if(face_compose_queue.empty())return;}
            if(!owner.try_lock())return;}}   // the queuer holds it now and composes
    FaceOwnerTurn(const FaceOwnerTurn&)=delete;FaceOwnerTurn& operator=(const FaceOwnerTurn&)=delete;
};
using FaceCpuUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
FaceCpuUpdate face_cpu_update_original{};
uintptr_t FaceCpuPointer(uintptr_t at) {
    uintptr_t value{};if(!Read(at,&value,8))throw std::runtime_error("native FaceGen CPU pointer");return value;
}
uintptr_t __fastcall InitializeNativeFaceImage(uintptr_t receiver,uintptr_t input,uintptr_t material_source) noexcept {
    try {
        const FaceOwnerTurn owner(true);const std::lock_guard hold(face_image_lock);
        if(face_images_closed || face_images.contains(receiver))throw std::runtime_error("native FaceGen owner admission");
        // Actual835820's 0xA0 prefix and CPU initialization. Old GPU resources
        //20/28 and old texture-set30 remain unavailable, never fake handles.
        std::memset(reinterpret_cast<void*>(receiver),0,0xA0);
        *reinterpret_cast<uintptr_t*>(receiver)=image+0x2843378;
        *reinterpret_cast<uintptr_t*>(receiver+8)=image+0x2843358;
        *reinterpret_cast<uintptr_t*>(receiver+0x38)=input;
        *reinterpret_cast<uint32_t*>(receiver+0x94)=1;
        const auto bank=FaceCpuPointer(image+0x477DA08);
        if(!bank)throw std::runtime_error("native FaceGen authored image bank");
        const BYTE variant=input?reinterpret_cast<BYTE(__fastcall*)(uintptr_t)>(image+0x831110)(input):0;
        auto source=FaceCpuPointer(bank+(variant?0x20:0x18));
        if(!source)source=FaceCpuPointer(bank+0x18);
        uint32_t width{},height{};
        if(source && source!=uintptr_t(-8)) {
            if(!Read(source+0x10,&width,4) || !Read(source+0x14,&height,4) ||
               width>4096 || height>4096 || uint64_t(width)*height*4>64u*1024u*1024u)
                throw std::runtime_error("native FaceGen authored image dimensions");
            // Pure836720 owns real engine-heap CPU storage; original GPU
            // construction is not required by the actual8362C0 producer.
            reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t,uint32_t)>(image+0x836720)(receiver+8,width,height);
        }
        face_images.emplace(receiver,NativeFaceImage{input,material_source,{}});
        char row[320]{};sprintf_s(row,
            "{\"event\":\"native_face_image_initialized\",\"width\":%u,\"height\":%u,\"actual_cpu_storage\":true,\"current_image_ready\":false,\"legacy_gpu_slots\":false,\"secondary_face_composition_available\":false}\n",width,height);Log(row);
        return receiver;
    }catch(...){Refuse("native_face_image_initialization_failed");}
}
// I-0097 tranche 2 E5 (i97-t2-obs-3: at each area crossing a character's face image was made again, DS3's FaceGen producer and
// the texture's upload taking about 4 ms under face_image_lock, while the collect's lookup of another face, NativeComposedFace,
// waited for it): the owner is checked under the lock, DS3's producer, the pixels' copy and the upload run without it, and the
// image is published under it. Step review: the owners' hooks keep their turns under face_owner_lock (the update and the
// composition run on different threads and share the status +0x70).
void __fastcall UpdateNativeFaceImage(uintptr_t receiver,uintptr_t second,uintptr_t third,uintptr_t fourth) noexcept {
    try {
        const FaceOwnerTurn owner(true);
        {const std::shared_lock hold(face_image_lock);
            if(!face_images.contains(receiver) || face_images_closed || !face_cpu_update_original)
                throw std::runtime_error("native FaceGen current CPU owner");}
        uint32_t prior{};BYTE requested{};
        if(!Read(receiver+0x70,&prior,4) || !Read(receiver+0x90,&requested,1))
            throw std::runtime_error("native FaceGen CPU request");
        face_cpu_update_original(receiver,second,third,fourth);
        uint32_t ready{},width{},height{};
        if(!Read(receiver+0x70,&ready,4))throw std::runtime_error("native FaceGen producer status");
        if(ready!=2 || !requested || prior!=0)return;
        if(!Read(receiver+0x10,&width,4) || !Read(receiver+0x14,&height,4) ||
           !width || !height || width>4096 || height>4096)
            throw std::runtime_error("native FaceGen current image dimensions");
        ds3content::CpuTextureResource content;content.name="current-engine-face-image";
        content.width=width;content.height=height;content.mip_levels=content.array_slices=1;
        // Actual835820 asks836C40 for DXGI29, one mip; no TPF view inference.
        content.format=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        ds3content::CpuTextureSubresource plane;plane.width=width;plane.height=height;
        plane.row_bytes=width*4;plane.rows=height;plane.bytes.resize(size_t(width)*height*4);
        if(!Read(FaceCpuPointer(receiver+0x18),plane.bytes.data(),plane.bytes.size()))
            throw std::runtime_error("native FaceGen current CPU pixels");
        content.subresources.push_back(std::move(plane));
        auto current=textures.AdmitOwned(receiver,std::move(content));
        {const std::lock_guard hold(face_image_lock);const auto found=face_images.find(receiver);
            if(found==face_images.end() || face_images_closed)throw std::runtime_error("native FaceGen current CPU owner");
            found->second.current=current;}
        char row[320]{};sprintf_s(row,
            "{\"event\":\"native_face_image_updated\",\"generation\":%llu,\"bytes\":%llu,\"actual_cpu_pixels\":true,\"shared_native_resource\":true,\"legacy_gpu_slots\":false,\"secondary_face_composition_available\":false,\"drawn\":false}\n",current->generation,current->content_bytes);Log(row);
    }catch(...){Refuse("native_face_image_update_failed");}
}
void __fastcall RetireNativeFaceImage(uintptr_t receiver) noexcept {
    try {
        const FaceOwnerTurn owner(true);const std::lock_guard hold(face_image_lock);
        for(const auto offset:{0x20u,0x28u,0x30u})if(FaceCpuPointer(receiver+offset))
            throw std::runtime_error("native FaceGen foreign GPU or texture-set owner");
        textures.Retire(receiver);textures.Retire(receiver+0x28);face_images.erase(receiver);
        // Pure base-image destruction frees the genuine CPU allocation only.
        reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uint32_t)>(image+0x835D60)(receiver+8,0);
        *reinterpret_cast<uintptr_t*>(receiver)=image+0x2843378;
    }catch(...){Refuse("native_face_image_retirement_failed");}
}
// Actual836E40 (callers C36FC0/C3F980 after 8368C0): at status +70 == 2 it
// sets 3, uploads the CPU image +18 into +20, draws it over the whole 512x512
// target +28 with the 2D drawer, then the mark layer (+40 texture, centre
// +48/+4C, angle +50, scale +54, colour +60) when +40 is set; it ends with +70 = 0
// and +91 = 1, which CFDE70 reads before passing texture set +30 to the draw.
// Native executes both layers on its own queue (the mark: ComposeNativeFaceLocked, I-0113 tranche 16).
void __fastcall ComposeNativeFaceImage(uintptr_t receiver) noexcept {
    try {
        {FaceOwnerTurn turn(false);
            if(turn.owner.owns_lock()){ComposeNativeFaceLocked(receiver,false);return;}}
        {const std::lock_guard hold(face_compose_queue_lock);
            if(std::find(face_compose_queue.begin(),face_compose_queue.end(),receiver)==face_compose_queue.end())face_compose_queue.push_back(receiver);}
        FaceOwnerTurn retry(false);   // the holder may have released before the queueing: then this turn composes it
    }catch(...){Refuse("native_face_composition_failed");}
}
// CFDE70: a part object's FgTextureIns (+48) supplies its texture set only after
// composition (+91). D39170 marks the model texture whose path is FG_face_gen
// (flag 0x80, default SYSTEX_Dummy); 51B5C0 binds the target +28 there.
std::shared_ptr<const bootstrap::NativeTextureRecord> NativeComposedFace(uintptr_t owner) {
    if(!owner)return nullptr;BYTE composed{};if(!Read(owner+0x91,&composed,1) || !composed)return nullptr;
    const std::shared_lock hold(face_image_lock);const auto found=face_images.find(owner);
    return found==face_images.end() || face_images_closed?nullptr:found->second.composed;
}
void CloseNativeFaceImages() {
    const FaceOwnerTurn owners(true);const std::lock_guard hold(face_image_lock);face_images_closed=true;
    for(auto& [receiver,owner]:face_images){textures.Retire(receiver);textures.Retire(receiver+0x28);owner.current.reset();owner.composed.reset();}
    // Engine CPU prefixes remain caller-owned until their actual destruction.
}
// D3F7C0/51B940: per-instance physical vertex streams. The caller's +10
// remains nil: it is an old GX buffer array, not a native resource handle.
struct NativeModelStream {
    uint32_t mesh{},stride{},vertices{},position_format{},position_offset{};
    uint32_t normal_format{},normal_offset{},extra_format{},extra_offset{};
    std::vector<BYTE> bytes;
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    std::shared_ptr<const UploadVersion> version;   // cD5: the pooled buffer behind `resource` (empty on the legacy path)
};
struct NativeModelStreamVersion {uint64_t generation{};std::vector<NativeModelStream> streams;};
struct NativeModelStreamOwner {
    uintptr_t model{};int32_t meshes{};
    std::shared_ptr<const NativeModelStorage> storage;
    std::vector<NativeModelStream> authored;
    std::shared_ptr<const NativeModelStreamVersion> current;
};
std::mutex model_stream_lock;
std::unordered_map<uintptr_t,NativeModelStreamOwner> model_streams;
uint64_t model_stream_generation{};
bool model_streams_closed{};
template<class T> T ModelStreamField(uintptr_t at) {
    T value{};if(!Read(at,&value,sizeof(value)))throw std::runtime_error("native model stream CPU input");return value;
}
void __fastcall RetireNativeModelStreams(uintptr_t receiver) noexcept {
    census::NoteRetirement(census::StreamRetire,_ReturnAddress());   // R13 cR0
    try {
        const std::lock_guard hold(model_stream_lock);
        if(ModelStreamField<uintptr_t>(receiver+0x10))Refuse("native_model_stream_original_gpu_present");
        const auto found=model_streams.find(receiver);
        if(found!=model_streams.end()) {
            // No old GX fence or array release. B6 (R3): the owner is kept until the queue completes what was submitted
            // until now (no wait); draws and ingress lists retain the versions they use.
            RetireWhenSubmittedWorkCompletes(std::make_shared<NativeModelStreamOwner>(std::move(found->second)));model_streams.erase(found);
            static LONG64 retired{};if(InterlockedIncrement64(&retired)<=64){char row[200]{};sprintf_s(row,"{\"event\":\"native_model_stream_retired\",\"receiver\":%llu,\"caller\":%lld}\n",
                static_cast<unsigned long long>(receiver),static_cast<long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);}
        }
        *reinterpret_cast<BYTE*>(receiver+0x1C)=0;
    }catch(...){Refuse("native_model_stream_retirement_failed");}
}
void CloseNativeModelStreams() {
    // R13 cF.4 (phase F sheet 3.3, PILOT D-2): the flag under the stream lock, released before the submission lock, so
    // model_stream_lock is never held while gpu_submissions is taken (Clear() takes them in the other order through the
    // menu-view fallback collect). Closed, the map admits nothing; it is emptied after the drain.
    {const std::lock_guard hold(model_stream_lock);model_streams_closed=true;}
    {const std::lock_guard submission(gpu_submissions);gpu.Drain(WaitClose);}
    std::unordered_map<uintptr_t,NativeModelStreamOwner> closed;
    {const std::lock_guard hold(model_stream_lock);closed.swap(model_streams);}
}

void __fastcall InitializeNativeModelStreams(uintptr_t receiver) noexcept {
    census::Note(census::StreamInit);   // R13 cR0
    try {
        {const census::Internal internal;RetireNativeModelStreams(receiver);} // genuine derived constructor reinitializes twice
        NativeModelStreamOwner owner;owner.model=ModelStreamField<uintptr_t>(receiver+8);
        if(!owner.model)return;
        {const std::lock_guard hold(model_stream_lock);if(model_streams_closed){   // A7 (the final review's L8): checked before the copies too
            *reinterpret_cast<int32_t*>(receiver+0x18)=ModelStreamField<int32_t>(owner.model+0xA0);return;}}
        owner.meshes=ModelStreamField<int32_t>(owner.model+0xA0);
        if(owner.meshes<0 || owner.meshes>4096)throw std::runtime_error("native model stream mesh extent");
        const auto storage=ModelStreamField<uintptr_t>(owner.model+0xE8);
        {const std::shared_lock hold(model_storage_lock);const auto found=model_storage.find(storage);   // I-0081: a lookup, shared
            if(found==model_storage.end())throw std::runtime_error("native model stream storage owner");owner.storage=found->second;}
        const auto meshes=ModelStreamField<uintptr_t>(owner.model+0xA8);
        const auto vertices=ModelStreamField<uintptr_t>(owner.model+0x158);
        const auto vertex_count=ModelStreamField<int32_t>(owner.model+0x150);
        const auto layouts=ModelStreamField<uintptr_t>(owner.model+0x180);
        uint32_t missing{};
        for(int32_t mesh=0;mesh<owner.meshes;++mesh) {
            const auto entry=meshes+uint64_t(mesh)*0x48;
            const auto count=ModelStreamField<uint16_t>(entry+0x2A);
            const auto indices=ModelStreamField<uintptr_t>(entry+0x38);
            if(count>32)throw std::runtime_error("native model stream count");
            bool copied{};
            for(uint32_t slot=0;slot<count && !copied;++slot) {
                const auto index=ModelStreamField<int32_t>(indices+slot*4);
                if(index<0 || index>=vertex_count)throw std::runtime_error("native model stream index");
                const auto header=vertices+uint64_t(index)*0x90;
                const auto layout_index=ModelStreamField<int32_t>(header+0x64);
                if(layout_index<0 || layout_index>4096)throw std::runtime_error("native model stream layout index");
                const auto layout=layouts+uint64_t(layout_index)*0x18;
                const auto elements=ModelStreamField<uintptr_t>(layout);
                const auto elements_count=ModelStreamField<int32_t>(layout+8);
                if(elements_count<0 || elements_count>128)throw std::runtime_error("native model stream layout extent");
                NativeModelStream stream;stream.mesh=mesh;
                stream.stride=ModelStreamField<uint32_t>(header+0x68);
                stream.vertices=ModelStreamField<uint32_t>(header+0x7C);
                bool position{};
                for(int32_t element=0;element<elements_count;++element) {
                    std::array<uint32_t,7> fields{};
                    if(!Read(elements+uint64_t(element)*0x1C,fields.data(),sizeof(fields)))throw std::runtime_error("native model stream element");
                    if(fields[1])continue;
                    if(fields[0]==0 && (fields[2]==2 || fields[2]==6)) {
                        position=true;stream.position_format=fields[2];stream.position_offset=fields[4];
                    }else if(fields[0]==3 && (fields[2]==2 || fields[2]==6 || fields[2]==0x1E)) {
                        stream.normal_format=fields[2];stream.normal_offset=fields[4];
                    }else if(fields[0]==9 && fields[2]==0x1C) {
                        stream.extra_format=fields[2];stream.extra_offset=fields[4];
                    }
                }
                if(!position)continue;
                // First position stream is the genuine constructor's choice.
                // Other streams stay authored; the first selected renderer
                // family still requires one stream per mesh.
                const auto offset=ModelStreamField<uint32_t>(header+0x78);
                const uint64_t bytes=uint64_t(stream.vertices)*stream.stride;
                if(!stream.stride || stream.stride>4096 || bytes>32u*1024u*1024u ||
                   offset>owner.storage->bytes.size() || bytes>owner.storage->bytes.size()-offset ||
                   ModelStreamField<uintptr_t>(header+0x70)!=storage ||
                   ModelStreamField<int32_t>(header+0x6C)!=0 ||
                   (stream.position_format==6 && (stream.stride<12 || stream.position_offset>stream.stride-12)) ||
                   (stream.normal_format==0x1E && (stream.stride<4 || stream.normal_offset>stream.stride-4)) ||
                   (stream.extra_format==0x1C && (stream.stride<4 || stream.extra_offset>stream.stride-4)))
                    throw std::runtime_error("native model stream authored bytes");
                stream.bytes.assign(owner.storage->bytes.begin()+offset,owner.storage->bytes.begin()+offset+static_cast<size_t>(bytes));
                owner.authored.push_back(std::move(stream));copied=true;
            }
            if(!copied)++missing;
        }
        const auto owned=owner.authored.size();
        // The mesh count the genuine constructor sets, written first (I-0137's step review L3: also when the streams are closed).
        *reinterpret_cast<int32_t*>(receiver+0x18)=ModelStreamField<int32_t>(ModelStreamField<uintptr_t>(receiver+8)+0xA0);
        {const std::lock_guard hold(model_stream_lock);if(model_streams_closed)return;   // I-0120 A7: after the close, nothing to own (as the cloth path)
            model_streams.emplace(receiver,std::move(owner));}
        // 1089: receiver, model, receiver class and caller, to join with character-part patches.
        uintptr_t table{};Read(receiver,&table,8);
        char row[420]{};sprintf_s(row,"{\"event\":\"native_model_stream_initialized\",\"mesh_streams\":%llu,\"missing_position_meshes\":%u,\"current_physical_ready\":false,\"legacy_gpu_array\":false,\"receiver\":%llu,\"model\":%llu,\"table_rva\":%lld,\"caller\":%lld}\n",
            static_cast<unsigned long long>(owned),missing,static_cast<unsigned long long>(receiver),
            static_cast<unsigned long long>(ModelStreamField<uintptr_t>(receiver+8)),static_cast<long long>(table-image),
            static_cast<long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-image));Log(row);
    }catch(const std::exception& error){char row[320]{};sprintf_s(row,"{\"event\":\"native_model_stream_error\",\"reason\":\"%s\"}\n",error.what());Log(row);Refuse("native_model_stream_initialization_failed");}
}
// The engine's CPU lock [image+0x4793C48] (table 39F76B0: +0x10 EnterCriticalSection, its timeout -1, +0x18 TryEnter, +0x20 Leave;
// I-0105 native_engine_lock_table and GhidraMCP): recursive, taken before any native lock (the producers hold it when they reach
// a native hook). Its users are the nine functions that read its table (51B940, 836E40, DFD6A0, D009E0, D00E60, D00F00, D00FF0,
// D071B0, D07220: draw-entity slots and stream updates), none reached from a native path holding decal_cpu_lock (I-0105 RESULT).
// A try (TryEnter answers 0 when it took it) leaves `held` false when another thread holds it.
struct EngineCpuLock {
    uintptr_t object{image+0x4793C48},table{ModelStreamField<uintptr_t>(object)},leave{ModelStreamField<uintptr_t>(table+0x20)};bool held{};
    explicit EngineCpuLock(bool try_only=false) {
        if(try_only)held=reinterpret_cast<int32_t(__fastcall*)(uintptr_t)>(ModelStreamField<uintptr_t>(table+0x18))(object)==0;
        else{reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t)>(ModelStreamField<uintptr_t>(table+0x10))(object,UINT32_MAX);held=true;}}
    ~EngineCpuLock() {if(held)reinterpret_cast<void(__fastcall*)(uintptr_t)>(leave)(object);}   // read at construction: the destructor never throws
    EngineCpuLock(const EngineCpuLock&)=delete;EngineCpuLock& operator=(const EngineCpuLock&)=delete;
};
void __fastcall UpdateNativePhysicalModelStreams(uintptr_t receiver) noexcept {
    census::Note(census::StreamUpdate);   // R13 cR0
    try {
        if(!ModelStreamField<uintptr_t>(receiver+8))return;
        // Preserve the original CPU producer's lock, not its old GPU mapping.
        const EngineCpuLock cpu_lock;
        const std::lock_guard hold(model_stream_lock);if(model_streams_closed)return;   // I-0120 A7: a late update after the close
        const auto found=model_streams.find(receiver);
        if(found==model_streams.end())throw std::runtime_error("native physical stream owner");
        auto& owner=found->second;
        const auto remap=ModelStreamField<uintptr_t>(receiver+0x20);
        const auto physics=ModelStreamField<uintptr_t>(receiver+0x28);
        if(!remap)return; // original per-mesh CPU update is gated by this input
        if(!physics)throw std::runtime_error("native physical stream source");
        const auto positions=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x833DF0)(physics);
        const auto normals=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x833DE0)(physics);
        auto version=std::make_shared<NativeModelStreamVersion>();version->streams=owner.authored;
        for(auto& stream:version->streams)for(uint32_t vertex=0;vertex<stream.vertices;++vertex) {
            const auto source=reinterpret_cast<int32_t(__fastcall*)(uintptr_t,uint32_t,uint32_t)>(image+0x773590)(remap,stream.mesh,vertex);
            if(source<0 || source>1048576)throw std::runtime_error("native physical position index");
            auto* destination=stream.bytes.data()+size_t(vertex)*stream.stride;
            if(stream.position_format==6) {
                std::array<float,3> position{};
                if(!Read(positions+uint64_t(source)*12,position.data(),sizeof(position)))throw std::runtime_error("native physical positions");
                for(const auto value:position)if(!std::isfinite(value))throw std::runtime_error("native physical position nonfinite");
                std::memcpy(destination+stream.position_offset,position.data(),sizeof(position));
            }
            if(stream.normal_format) {
                auto normal=reinterpret_cast<uint32_t(__fastcall*)(uintptr_t,uint32_t,uint32_t)>(image+0x7735F0)(remap,stream.mesh,vertex);
                if(!normal)normal=ModelStreamField<uint32_t>(normals+uint64_t(source)*4);
                if(stream.normal_format==0x1E)std::memcpy(destination+stream.normal_offset,&normal,4);
            }
            if(stream.extra_format==0x1C) {
                const auto extra=reinterpret_cast<uint32_t(__fastcall*)(uintptr_t,uint32_t,uint32_t)>(image+0x7735C0)(remap,stream.mesh,vertex);
                std::memcpy(destination+stream.extra_offset,&extra,4);
            }
        }
        // B6: no frame lock (no queue work); model_stream_lock, held here, keeps the device alive against the close.
        for(auto& stream:version->streams)if(!stream.bytes.empty()){stream.resource=PublishNativeStreamBytes(stream.bytes,stream.version);
            gpu_audit.dynamic_stream_updates.fetch_add(1,std::memory_order_relaxed);gpu_audit.dynamic_stream_bytes.fetch_add(stream.bytes.size(),std::memory_order_relaxed);}
        version->generation=++model_stream_generation;owner.current=version;
        static LONG64 logged{};if(InterlockedIncrement64(&logged)<=8){char row[320]{};sprintf_s(row,
            "{\"event\":\"native_model_stream_updated\",\"generation\":%llu,\"mesh_streams\":%llu,\"current_cpu_vertices\":true,\"native_resources\":true,\"legacy_gpu_array\":false,\"drawn\":false}\n",version->generation,static_cast<unsigned long long>(version->streams.size()));Log(row);}
    }catch(...){Refuse("native_physical_model_stream_update_failed");}
}
struct NativeModelPosePrefix {
    uintptr_t gpu[4]{},matrices[2]{},reserved[7]{};
    int32_t index{},ring{},count{};
    BYTE fast{},single{},cloth{},padding{};
    uintptr_t heap{},model{};
};
static_assert(sizeof(NativeModelPosePrefix)==0x88 &&
    offsetof(NativeModelPosePrefix,index)==0x68 && offsetof(NativeModelPosePrefix,model)==0x80);
using PoseRow=std::array<float,12>;
struct NativeModelPoseSnapshot {
    uint64_t generation{};
    std::vector<PoseRow> matrices;
    std::shared_ptr<const ds3content::ModelPoseVersion> palettes;
};
struct NativeModelPoseOwner {
    uintptr_t model{};
    int32_t ring{},count{};
    std::array<std::vector<PoseRow>,2> matrices;
    bool auxiliary_present{};
    std::shared_ptr<ds3content::ModelPoseOwner> palettes;
    std::shared_ptr<const NativeModelPoseSnapshot> current;
    // 1186 (VELOCITY-SKINNING §2-4): the version the ring slot index-1 still holds, b6 cbPrevObjMatrix of a Velo draw. With
    // ring 2 an advancing update moves the current version here; ring 1 (no keepPrevious) has none (b6 = b5, zero skinned
    // velocity); before the second update it is unset and the current version serves (previous = current on the first frame).
    std::shared_ptr<const NativeModelPoseSnapshot> previous;
    uintptr_t init_caller{}; // 1070 witness: caller RVA of the 15490 initialization
    uint64_t updates{};
    uint64_t nonfinite{};   // I-0101: updates whose rows were not all finite (the diagnostic row's rate)
};
std::shared_mutex model_pose_lock;   // I-0078: shared by the admission workers' lookups
std::unordered_map<uintptr_t,NativeModelPoseOwner> model_poses;
// 1107: native union of a character-part model's material policy flags (the +224 word the original model setup
// stores and native admission leaves at 0). Bit 14 (0x4000) gives its poses the auxiliary/cancel palette pair.
std::mutex model_policy_lock;
std::unordered_map<uintptr_t,uint32_t> model_policy_flags;
// D-0193 (D2 not shown in Thomas's play: 13 of 19 paint requests refused by the receiver gate): the same union by the model's
// GXBloodyMaterialPatch (the pointer at the receiver +0x50). 51F80 copies bits 2 and 3 of the model's +224 into the receiver's gates +0x78/+0x79
// when it builds the patch; natively +224 is still 0 then, so the paint drain takes the gates from this union. Under model_policy_lock.
std::unordered_map<uintptr_t,uint32_t> native_patch_flags;
std::atomic<uint64_t> model_pose_generation{};   // I-0105: the model particles' poses take theirs outside model_pose_lock
}  // namespace
