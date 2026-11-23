// Effects: the FX world, the SFX manager, the native FFX replay, RecordNativeEffects, the scene control and the SFX
// start-up and asset loading. Part of game_bootstrap.cpp's single translation unit (the order is there): it relies on every
// earlier part (20_startup_phases.inc before it).
#pragma once
#include "20_startup_phases.inc"
namespace {
// FX world (942-949): the effect system 24B260 also owns three CPU structures
// that character code uses directly: the FX world 4962360 (248CC0), its light
// registry 4962398 (2594E0/2596D0) and its node manager 49623A0 (25B280/25B370).
// Character lights (CE8090/CE8670/CE8730) and model FX nodes (D05C80, D3C4D0,
// 66F7A0...) register there. They are built here genuinely, on 24B260's heap
// 479C270, without the GPU parts of 248E30 (per-buffer random textures/views
// +0x40/+0x50/+0x58 and the 25D220/25E960 sub-objects +0x18/+0x20): no GX code
// that would consume those runs natively. Lights stay CPU records for the
// native renderer; nothing draws them yet.
uintptr_t native_fx_world{};
void PublishNativeFxWorld() {
    uintptr_t world{},registry{},nodes{},heap{};
    if(!Read(image+0x4962360,&world,8) || world || !Read(image+0x4962398,&registry,8) || registry ||
       !Read(image+0x49623A0,&nodes,8) || nodes || native_fx_world)Refuse("native_fx_world_not_pristine");
    if(!Read(image+0x479C270,&heap,8) || !heap)Refuse("native_fx_world_heap");
    const auto allocate=[&](size_t bytes,size_t alignment){
        const auto storage=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(bytes,alignment,heap);
        if(!storage)throw std::bad_alloc();return storage;};
    // 255090: light registry, then the world.
    registry=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x2594E0)(allocate(0x120,0x10));
    if(!registry || !Read(image+0x4962398,&nodes,8) || nodes!=registry)Refuse("native_fx_light_registry");
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0x2596D0)(registry,heap);
    world=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x248CC0)(allocate(0x1C0,0x10));
    uintptr_t published{};
    if(!world || !Read(image+0x4962360,&published,8) || published!=world)Refuse("native_fx_world_constructor");
    // 248E30's CPU stores: heap, seed, xorshift state, the 400000-byte id table.
    const auto q=[&](uint32_t offset,uint64_t value){*reinterpret_cast<uint64_t*>(world+offset)=value;};
    q(0x10,heap);
    alignas(16) std::array<BYTE,64> random{};const auto rng=reinterpret_cast<uintptr_t>(random.data());
    reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x17A4DE0)(rng);
    const auto next=[&]{reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x17AFFC0)(rng);return *reinterpret_cast<uint64_t*>(rng+8);};
    const auto a=next()>>0x2D,b=next()>>0x27,c=next()>>0x22,low=next()>>0xC;
    const auto seed=static_cast<int32_t>(((a&0x3F)+1)*((b&0x3F)+1)*((c&0x1F)*1000+1000)+(low&0x3FF));
    *reinterpret_cast<int32_t*>(world+0x2C)=seed;
    reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t)>(image+0x17B0040)(world+0x30,seed);
    *reinterpret_cast<int32_t*>(world+0x28)=0; // no per-buffer GPU random textures
    uintptr_t heap_table{};if(!Read(heap,&heap_table,8) || !heap_table)Refuse("native_fx_world_heap_table");
    const auto ids=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,size_t,size_t)>(*reinterpret_cast<uintptr_t*>(heap_table+0x50))(heap,400000,0x10);
    if(!ids)throw std::bad_alloc();
    std::memset(reinterpret_cast<void*>(ids),0xFF,400000);q(0x48,ids);
    q(0x08,reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x259C60)(registry));
    // 24B260 after 255090: the node manager.
    nodes=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x25B280)(allocate(0x90,8));
    if(!nodes || !Read(image+0x49623A0,&published,8) || published!=nodes)Refuse("native_fx_node_manager");
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0x25B370)(nodes,heap);
    // FFX-DRAW E07 (on by default, no-ffx-gpu.flag opts out): 248E30's GPU parts on the effect device (its display reads are redirected, so
    // its GX buffers and views become native records): the GPU-particle constant holder +0x18 (25D220: gcbSceneParam 0x1C0 twice,
    // ccbSceneParam 0x450, ccbForceFiledParam 0xDA0, 0x10, 0x20, ccbEmitionSeed 0x2F0) and +0x20 (25E960: 0xA0, 0xE0), and one
    // context's random seed buffer (+0x28 = 1 context: +0x40 its xorshift state, +0x50 the 0x400-byte buffer of 64 random uint4,
    // +0x58 its view), as 248E30 builds them per GX context.
    static const bool gpu_particles=native_ffx_draw && !ModuleFlag(L"no-ffx-gpu.flag");   // E07 on by default (D-0186)
    if(gpu_particles){
        const auto holder=allocate(0x38,8);std::memset(reinterpret_cast<void*>(holder),0,0x38);q(0x18,holder);
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x25D220)(holder);
        const auto second=allocate(0x10,8);std::memset(reinterpret_cast<void*>(second),0,0x10);q(0x20,second);
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x25E960)(second);
        *reinterpret_cast<int32_t*>(world+0x28)=1;
        const auto states=allocate(8,0x10),buffers=allocate(8,0x10),views=allocate(8,0x10),state=allocate(0x14,4);
        *reinterpret_cast<int32_t*>(state)=seed;reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t)>(image+0x17B0040)(state+4,seed);
        q(0x40,states);*reinterpret_cast<uintptr_t*>(states)=state;q(0x50,buffers);*reinterpret_cast<uintptr_t*>(buffers)=0;q(0x58,views);*reinterpret_cast<uintptr_t*>(views)=0;
        alignas(16) std::array<uint32_t,256> seeds_words{};auto* x=reinterpret_cast<uint32_t*>(world+0x30);
        for(auto& word:seeds_words){const uint32_t t=x[0]<<11^x[0];x[0]=x[1];x[1]=x[2];x[2]=x[3];x[3]=(x[3]>>11^t)>>8^x[3]^t;word=x[3];}
        const auto device=*reinterpret_cast<const uintptr_t*>(effect_system::Display());   // the effect display's +0: the device token
        alignas(16) std::array<uint32_t,6> description{0x400,0,0x80,0,0x20,0x400};std::array<uintptr_t,3> initial{reinterpret_cast<uintptr_t>(seeds_words.data()),0,0};
        const auto created=reinterpret_cast<uint32_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t)>(image+0x19254F0)(device,buffers,
            reinterpret_cast<uintptr_t>(description.data()),reinterpret_cast<uintptr_t>(initial.data()));
        alignas(16) std::array<uint32_t,8> view{0,1,0,1,0,0,0,0};
        const auto viewed=created?1u:reinterpret_cast<uint32_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t)>(image+0x1925BF0)(device,views,
            *reinterpret_cast<const uintptr_t*>(buffers),reinterpret_cast<uintptr_t>(view.data()));
        uintptr_t words[9]{};Read(holder,words,0x38);Read(second,words+7,0x10);
        int made{};for(const auto w:words)made+=effect_system::IsEffectObject(w)?1:0;
        char row[200]{};sprintf_s(row,"{\"event\":\"native_fx_world_gpu\",\"constant_buffers\":%d,\"random_buffer\":%u,\"random_view\":%u}\n",made,created,viewed);Log(row);}
    native_fx_world=world;
    BYTE enabled{};Read(registry+0x110,&enabled,1);
    char row[288]{};sprintf_s(row,"{\"event\":\"native_fx_world_published\",\"genuine_cpu\":true,\"light_registry_enabled\":%u,\"gpu_random_textures\":false,\"fx_rendering\":false}\n",enabled);Log(row);
}
// E6C9D0 without its GX members (see native_sfx_manager). Every write below is
// one of E6C9D0's own CPU stores, in its order; omitted members stay null.
void PublishNativeSfxManager() {
    uintptr_t prior{},heap{},list_heap{};
    if(!Read(image+0x4799C98,&prior,8) || prior || native_sfx_manager)Refuse("native_sfx_manager_not_pristine");
    {uintptr_t stop_table{};float one{};   // I-0141 13d: the stop table's global and its value, checked before any allocation
        if(!Read(image+0x4773698,&stop_table,8) || stop_table)Refuse("native_sfx_manager_not_pristine");
        if(!Read(image+0x3D7EE10,&one,4) || one!=1.f)Refuse("native_sfx_stop_table_scope");}
    if(!Read(image+0x479C270,&heap,8) || !heap || !Read(image+0x479C288,&list_heap,8) || !list_heap)
        Refuse("native_sfx_manager_heaps");
    const auto storage=reinterpret_cast<uintptr_t(__fastcall*)(size_t,size_t,uintptr_t)>(image+0x1777A00)(0xF0,0x10,heap);
    if(!storage)throw std::bad_alloc();
    std::memset(reinterpret_cast<void*>(storage),0,0xF0);
    const auto q=[&](uint32_t offset,uint64_t value){*reinterpret_cast<uint64_t*>(storage+offset)=value;};
    const auto d=[&](uint32_t offset,uint32_t value){*reinterpret_cast<uint32_t*>(storage+offset)=value;};
    q(0,image+0x2992EF8);
    using Node=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
    for(const uint32_t list:{0x08u,0x20u}){
        // The list heap's capability assertion (bit 5), then the sentinel node.
        uint64_t scratch[2]{};uintptr_t table{};
        if(!Read(list_heap,&table,8) || !table)Refuse("native_sfx_manager_list_heap");
        const auto caps=reinterpret_cast<const uint32_t*(__fastcall*)(uintptr_t,void*,uint32_t)>(*reinterpret_cast<uintptr_t*>(table+0x18))(list_heap,scratch,0);
        if(!caps || !((*caps>>5)&1))Refuse("native_sfx_manager_list_heap_capability");
        q(list+0x10,list_heap);
        const auto node=reinterpret_cast<Node>(image+0xE6ED20)(storage+list,0,0);
        if(!node)throw std::bad_alloc();
        q(list,node);
    }
    d(0x70,0x1010000);q(0x74,0x101);d(0x7C,0x41200000);
    uint64_t neutral{};std::array<BYTE,64> identity{};
    if(!Read(image+0x46402E0,&neutral,8) || !Read(image+0x39F9750,identity.data(),identity.size()))
        Refuse("native_sfx_manager_constants");
    // 810E90 is encrypted at rest: call it only once its decrypted code is present.
    static constexpr BYTE quality_code[24]{0x40,0x57,0x48,0x83,0xEC,0x70,0x48,0xC7,0x44,0x24,0x28,0xFE,0xFF,0xFF,0xFF,0x48,0x89,0x9C,0x24,0x80,0x00,0x00,0x00,0x48};
    BYTE live[sizeof(quality_code)]{};
    if(!Read(image+0x810E90,live,sizeof(live)) || std::memcmp(live,quality_code,sizeof(live)))Refuse("native_sfx_quality_code_not_decrypted");
    const int level=reinterpret_cast<int(__fastcall*)()>(image+0x810E90)();
    d(0xD0,static_cast<uint32_t>(level<0?0:(level<=2?level:2)));
    std::memcpy(reinterpret_cast<void*>(storage+0x80),identity.data(),identity.size());
    q(0xC0,neutral);q(0xC8,neutral);
    // I-0141 tranche 13d: the stop table (E6C9D0 140E6D050..140E6D08F, READ; first left out as a « colour table »): the FXR
    // external-value table {0: 1.0} made by 141AB0150(&key 0, 1, heap 479C270), its value set by 141AB05E0(table, 0, [3D7EE10]
    // = 1.0f), stored at +0x68 and at 4773698. Its gate: SprjSfxImp's entry update E6B210 calls E6B3C0, which, for an entry whose
    // stop bit (+0x14 bit 0) is set, tail-jumps to the entry's vt+0x40 with +0x68 only when +0x68 is not null (READ); vt+0x40
    // puts the table on the entry's handles through 141A87DF0 (SprjFD4MultiSfxIns, vtable 1429922C8: 140E6A750, each handle of
    // +0x70..+0x78, stride 0x460; SprjFD4SingleSfxIns, vtable 142992728: 140E6C1D0, its handle at +0x70 through 140244A50), so the
    // effects whose FXR state machine waits for « 1 <= external value 0 » (the ceremony's 732xxx) end by themselves (observed in
    // the original, i141-t13-x1). With +0x68 null a stopped entry never got the table and stayed in state 1 (its vt+0x20 still
    // called each frame) until a hard stop. The handle stop 1404D2AC0
    // and the other readers of 4773698 read the same table. CPU only (two allocations on heap 479C270: the object, its arrays);
    // the original's destructor E6D220 frees it, the native manager is never destroyed.
    {
        const float one=1.f;   // [3D7EE10], checked at the head
        int32_t key{};
        const auto table=reinterpret_cast<uintptr_t(__fastcall*)(const int32_t*,uint32_t,uintptr_t)>(image+0x1AB0150)(&key,1,heap);
        if(!table)throw std::bad_alloc();
        // The value goes in R8D as its bits (the constructor's MOV R8D,[RBP-0x29]), not in XMM2.
        uint32_t bits{};std::memcpy(&bits,&one,4);
        reinterpret_cast<void(__fastcall*)(uintptr_t,int32_t,uint32_t)>(image+0x1AB05E0)(table,0,bits);
        q(0x68,table);
        *reinterpret_cast<uintptr_t*>(image+0x4773698)=table;
    }
    native_sfx_manager=storage;
    *reinterpret_cast<uintptr_t*>(image+0x4799C98)=storage;
    char row[320]{};sprintf_s(row,"{\"event\":\"native_sfx_manager_published\",\"genuine_class\":true,\"quality\":%d,\"effect_system\":false,\"view_draw_list\":false,\"stop_table\":true,\"debug_menu\":false,\"sfx_draw_ready\":false}\n",level);Log(row);
}
// E6EA10/E6EC60 hand +0x58 to GX 24F2C0 unconditionally. Without effect system
// and view draw list their remaining genuine effect is none; the attachment is
// recorded so the native effect owner can take the view over later.
// W4 FW-3: once the effect system is published, 24F2C0's CPU statements run natively: the view is stored at sc+0x218 (null
// on detach); its two GXRenderToTexture targets, its depth copies and the display's view registration (+0xB18) are GX and
// are not made (draw-side, not wave 1).
void __fastcall NativeSfxViewAttach(uintptr_t manager,uintptr_t view) noexcept {
    uintptr_t system_member{},draw_list{};
    if(!manager || manager!=native_sfx_manager || !Read(manager+0x58,&system_member,8) ||
       (system_member && system_member!=native_effect_system.scene_control) ||
       !Read(manager+0x60,&draw_list,8) || draw_list)Refuse("native_sfx_view_attach_scope");
    if(!view)return;
    if(system_member)*reinterpret_cast<uintptr_t*>(system_member+0x218)=view;   // sc+0x210 (the view record's +0x74, read through the view's vt+0x18) is left 0 (inferred: only the draw side reads it)
    if(++sfx_view_attachments<=8){char row[224]{};sprintf_s(row,"{\"event\":\"native_sfx_view_attached\",\"count\":%llu,\"effect_system\":%s}\n",
        sfx_view_attachments,system_member?"true":"false");Log(row);}
}
void __fastcall NativeSfxViewDetach(uintptr_t manager,uintptr_t view) noexcept {
    uintptr_t system_member{},draw_list{};
    if(!manager || manager!=native_sfx_manager || !Read(manager+0x58,&system_member,8) ||
       (system_member && system_member!=native_effect_system.scene_control) ||
       !Read(manager+0x60,&draw_list,8) || draw_list)Refuse("native_sfx_view_detach_scope");
    if(!view)return;
    if(system_member)*reinterpret_cast<uintptr_t*>(system_member+0x218)=0;
    Log(system_member?"{\"event\":\"native_sfx_view_detached\",\"effect_system\":true}\n":"{\"event\":\"native_sfx_view_detached\",\"effect_system\":false}\n");
}
// FFX draw N5a (FFX-DRAW §2): the engine's effect recording into a native pass context. The original records its effect
// commands once a frame, in the scene-control draw 24E790 of GX pass 0x45 (Reference1927), through the pass context's command
// stream: 20E570([ctx+0x4B0]+0x38) allocates each 0x58-byte data block, BF4C0(ctx, 4, 0x18) the command {299E40, entity,
// data} with the key ctx+0x12C, BF2D0 commits. Natively one pass context (engine thread) is filled with the fields the
// recording reads (Reference1925-1927): +0x124 the pass 0x45, +0x410 a frustum whose vt+0x60 answers visible (explicit initial
// policy: no effect culling), +0x418 the view's camera, +0x250..+0x25C the camera's depth row, +0x4B0 a native sort block,
// +0x4B8/+0x4BC {1, 1}, +0x426 the reversal table (1F, 29, 2D, 31, 34), +0x4F0 a zeroed block (140089140 reads its +0x50),
// +0x58C 1; the three functions act on it natively (any other context runs the original). ffx-record.flag (opt-in rung).
namespace native_ffx {
// I-0141 tranche 15 (the haze in play at Archdragon Peak: the effects' own share in each mode): reference-no-ffx-draw.flag, a measuring
// tool for Reference runs alone, the original's counterpart of no-ffx-draw.flag. The scene control's effect draw 24E790 (sc, pass)
// returns at once, as its own early exits do (the GX display [144961AB0]'s +0x3E1 set, sc+0x5BE set, sc+0x20 clear, the pass's +0x124
// 0x3A); it returns nothing (READ, GhidraMCP 2026-11-21: every exit jumps to the epilogue 24EC8D, which sets neither RAX nor XMM0).
// The full path's last write, the byte [14496235C] = 0 at 24EC86 (24D000 sets it to 1 after a GX flush; GhidraMCP lists these two
// writes and no read, 2026-11-21), is kept where the four gates would have let the call through. The skipped body is more than the
// draw calls: the library's walk (sc+0x18 vt+0x20) and what its callees do (1ADDE80, the 299E40 callbacks, the GPU particles'
// 2A6A30), and on the +0x590 branch the pass context's +0x420..+0x12C writes and the command allocations, none read for effects
// beyond the draws: that the run differs from a plain Reference run by the effects' pictures alone is INFERRED (the original itself
// skips this body for about 80 ms at a cut, tranche 14), to check on the pictures. One row at the first skip.
using SceneDrawFn=void(__fastcall*)(uintptr_t,uintptr_t);SceneDrawFn reference_scene_draw_original{};std::atomic<bool> reference_draw_skipped{};
void __fastcall ReferenceSceneDrawSkipped(uintptr_t sc,uintptr_t pass) noexcept {
    uintptr_t display{};uint8_t shown{1},debug{1},on{};uint32_t id{0x3A};
    if(sc && pass && Read(image+0x4961AB0,&display,8) && display && Read(display+0x3E1,&shown,1) && Read(sc+0x5BE,&debug,1) && Read(sc+0x20,&on,1) &&
       Read(pass+0x124,&id,4) && !shown && !debug && on && id!=0x3A)*reinterpret_cast<volatile uint8_t*>(image+0x496235C)=0;
    if(!reference_draw_skipped.exchange(true))Log("{\"event\":\"reference_no_ffx_draw\",\"first_skip\":true}\n");}
struct Command {uintptr_t callback{},entity{},data{};uint32_t key{},pass{};};
alignas(16) std::array<uint8_t,0x600> pass_context{};alignas(16) std::array<uint8_t,0x100> sort_block{},pipeline_block{};
// 140089140(pipeline) gives each recorded draw its data +0x54, from which 299E40 sets the draw context's +0xAA8 = 1.5 / value (DAT_144962377),
// the factor of the effects' colour scale (slot 13 and its siblings): the world view's exposure in the original (Reference2039: 2.3 at Firelink,
// 2.0 at m45), 1.0 natively before (the zeroed pipeline block): the flames were drawn 2.3 times too bright (Native2040). For the native
// pass context's pipeline the native exposure (native_effect_exposure); any other pipeline goes to the original.
using PipelineExposure=float(__fastcall*)(uintptr_t);PipelineExposure pipeline_exposure_original{};
float __fastcall NativePipelineExposure(uintptr_t pipeline) noexcept {
    if(pipeline==reinterpret_cast<uintptr_t>(pipeline_block.data())){const float e=native_effect_exposure.load(std::memory_order_relaxed);if(e>0.f)return e;}
    return pipeline_exposure_original(pipeline);}
// The pass context's view volume +0x410: a GXViewVolume (0x2E0, as SceneCtrl+0x2A0) built by its CPU constructor 6E780 once and set
// from the camera record each frame by 6E9E0 (the setter the engine's views use: the projection fields, then the planes +0x160 by
// 6EB60), so slot 5's test (vt+0x60 = 6F310) culls as the original's main view does.
alignas(16) std::array<uint8_t,0x300> view_volume{};bool view_volume_built{};
std::deque<Command> commands;std::deque<std::array<uint8_t,0x80>> blocks;size_t blocks_used{};
uint64_t frames{};std::map<int,uint64_t> recorded;
uintptr_t Context() noexcept {return reinterpret_cast<uintptr_t>(pass_context.data());}
// I-0113 tranche 12 (gap 12: Flame Fan's model particle s08280 stays folded natively, its bones 2 to 5 at the bind pose), with
// ffx-record.flag: the FFX model animator (an FXR 605 anibnd): E6B950 creates it through the hook [144962380] (the stub 1402575E0
// returns 0 without it), its constructor E6B470 copies the model, anibnd and animation ids (+8, +0xC, +0x10: s08280's anibnd 8280, animation 10) and looks up the
// animation (+0x18), the animated skeleton (+0x20), its controls (+0x28) and the FLVER-to-Havok bone mapping (+0x30); Step E6B9D0
// samples it each update and sends the bone matrices as message 1000 to the effect's nodes (the model set's 1402823E0 poses its model instance).
// The first 16 constructions and the step count (at 1, 10, 100, 1000 ... calls) are written.
using AnimatorCreate=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);AnimatorCreate animator_create_original{};
using AnimatorConstruct=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);AnimatorConstruct animator_construct_original{};
using AnimatorStep=void(__fastcall*)(uintptr_t,uintptr_t,float,uintptr_t);AnimatorStep animator_step_original{};
std::atomic<uint64_t> animator_creates{},animator_constructs{},animator_steps{},animator_live_steps{};
uintptr_t __fastcall AnimatorCreateObserved(uintptr_t a,uintptr_t b,uintptr_t c,uintptr_t d) noexcept {
    const auto made=animator_create_original(a,b,c,d);const auto n=animator_creates.fetch_add(1)+1;
    if(n<=16 || !made){char row[160]{};sprintf_s(row,"{\"event\":\"native_ffx_animator_create\",\"call\":%llu,\"made\":%s}\n",n,made?"true":"false");Log(row);}
    return made;}
uintptr_t __fastcall AnimatorConstructObserved(uintptr_t self,uintptr_t owner,uintptr_t effect) noexcept {
    const auto result=animator_construct_original(self,owner,effect);const auto n=animator_constructs.fetch_add(1)+1;
    if(n<=16){int32_t ids[3]{};uintptr_t members[4]{};Read(self+8,&ids[0],4);Read(self+0xC,&ids[1],4);Read(self+0x10,&ids[2],4);Read(self+0x18,members,sizeof members);
        char row[320]{};sprintf_s(row,"{\"event\":\"native_ffx_animator\",\"call\":%llu,\"model\":%d,\"anibnd\":%d,\"animation\":%d,\"animation_handle\":%s,\"skeleton\":%s,\"controls\":%s,\"mapping\":%s}\n",
            n,ids[0],ids[1],ids[2],members[0]?"true":"false",members[1]?"true":"false",members[2]?"true":"false",members[3]?"true":"false");Log(row);}
    return result;}
void __fastcall AnimatorStepObserved(uintptr_t self,uintptr_t effect,float dt,uintptr_t context) noexcept {
    animator_step_original(self,effect,dt,context);
    uintptr_t members[2]{};Read(self+0x20,members,sizeof members);const bool live_step=members[0] && members[1];
    const auto n=animator_steps.fetch_add(1)+1;const auto live=live_step?animator_live_steps.fetch_add(1)+1:animator_live_steps.load();
    // A live animator's first 12 steps: the effect and the context 257660 needs (it sends nothing for a null context).
    // The message reaches the model set only when the update list's message item runs (141ADC190): the context's class and, for the
    // first live step, the return addresses above Step (its caller is reached through a table).
    if(n==1 || n==10 || n==100 || n==1000 || n==10000 || (live_step && live<=12)){uintptr_t table{};if(context)Read(context,&table,8);char row[240]{};
        sprintf_s(row,"{\"event\":\"native_ffx_animator_steps\",\"calls\":%llu,\"live\":%llu,\"this_live\":%d,\"dt\":%g,\"effect\":%s,\"context_class\":\"%llX\"}\n",
            n,live,int(live_step),dt,effect?"true":"false",static_cast<unsigned long long>(table>image?table-image:table));Log(row);
        if(live_step && live==1){void* chain[10]{};const auto depth=RtlCaptureStackBackTrace(0,10,chain,nullptr);std::string trace="{\"event\":\"native_ffx_animator_chain\",\"rvas\":[";
            for(USHORT i=0;i<depth;++i){char cell[24]{};sprintf_s(cell,"%s\"%llX\"",i?",":"",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(chain[i])-image));trace+=cell;}
            Log((trace+"]}\n").c_str());}}}
// 141ADC190(item, pass, message): the queued message's dispatch, the item a node's receiver (node+0x90), to the node's renderer +0x58
// (the model set's 1402823E0 for message 1000, the other renderers' 140267B20 answers false), with the renderer's class; counted.
using MessageDispatch=void(__fastcall*)(uintptr_t,uintptr_t,const uint32_t*);MessageDispatch message_dispatch_original{};
std::atomic<uint64_t> message_dispatches{};
void __fastcall MessageDispatchObserved(uintptr_t item,uintptr_t a2,const uint32_t* message) noexcept {
    const auto n=message_dispatches.fetch_add(1)+1;uint32_t type{};if(message)Read(reinterpret_cast<uintptr_t>(message),&type,4);
    if(n<=8 || n%1000==0){uintptr_t receiver{},table{};if(item && Read(item-0x90+0x58,&receiver,8) && receiver)Read(receiver,&table,8);char row[200]{};
        sprintf_s(row,"{\"event\":\"native_ffx_message_dispatch\",\"calls\":%llu,\"type\":%u,\"renderer\":\"%llX\"}\n",n,type,static_cast<unsigned long long>(table>image?table-image:table));Log(row);}
    message_dispatch_original(item,a2,message);}
// 257660(animator, effect, matrices, count, context): message 1000 queued when the effect's vt+8 is 0x898; 1402823E0 (the model
// set's slot 11, a node's renderer +0x58) receives it and poses the model instance. Their first 12 calls.
using AnimatorSend=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uintptr_t);AnimatorSend animator_send_original{};
using PoseMessage=uint64_t(__fastcall*)(uintptr_t,int32_t,const float*);PoseMessage pose_message_original{};
std::atomic<uint64_t> animator_sends{},pose_messages{};
void __fastcall AnimatorSendObserved(uintptr_t self,uintptr_t effect,uintptr_t matrices,uint32_t count,uintptr_t context) noexcept {
    int32_t type=-1;if(effect){uintptr_t table{},method{};if(Read(effect,&table,8) && table && Read(table+8,&method,8) && method)type=reinterpret_cast<int32_t(__fastcall*)(uintptr_t)>(method)(effect);}
    animator_send_original(self,effect,matrices,count,context);
    if(animator_sends.fetch_add(1)<12){char row[200]{};sprintf_s(row,"{\"event\":\"native_ffx_animator_send\",\"type\":\"%X\",\"count\":%u,\"matrices\":%s,\"context\":%s}\n",
        static_cast<unsigned>(type),count,matrices?"true":"false",context?"true":"false");Log(row);}}
uint64_t __fastcall PoseMessageObserved(uintptr_t set,int32_t message,const float* data) noexcept {
    const auto result=pose_message_original(set,message,data);
    if(message==1000 && pose_messages.fetch_add(1)<12){uint32_t count{};if(data)Read(reinterpret_cast<uintptr_t>(data),&count,4);
        char row[160]{};sprintf_s(row,"{\"event\":\"native_ffx_pose_message\",\"count\":%u}\n",count);Log(row);}
    return result;}
// The model particles' colour (s08280's mulCol was 1.533 times the original's, 1 / 0.652 = 2.3 / 1.5, until I-0113 tranche 13
// published the material's bit 20): 27EA70(set, draw context, effect,
// node) scales it by the context's +0xAA8 (299E40: 1.5 / exposure) when the set's +0x129 is set (27E5E0: 61A20 of the set's model,
// a material of its table +0x1F0 with +0x24 bit 20); its first 6 calls and the first 6 with +0x129 set.
using ModelSetDraw=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);ModelSetDraw model_set_draw_original{};
std::atomic<uint64_t> model_set_draws{};
uint64_t __fastcall ModelSetDrawObserved(uintptr_t set,uintptr_t context,uintptr_t effect,uintptr_t node) noexcept {
    float scale{};BYTE flag{};Read(context+0xAA8,&scale,4);Read(set+0x129,&flag,1);
    static std::atomic<uint64_t> flagged{};
    if(model_set_draws.fetch_add(1)<6 || (flag && flagged.fetch_add(1)<6)){
        char row[160]{};sprintf_s(row,"{\"event\":\"native_ffx_model_set_draw\",\"context_aa8\":%g,\"set_129\":%u}\n",scale,flag);Log(row);}
    return model_set_draw_original(set,context,effect,node);}
// Witness (both modes): the particle node update 1ADD510(node, pass, dt, emitter): +0x70 is the live particle count, the emitter's
// vt+8 the particles to emit this call. Counted per 3000 calls: calls, emitter absent, dt zero, the dt range, nodes with particles
// before and after, the emitted total, and the pause byte [1496237E] that zeroes the library's dt.
using NodeUpdate=void(__fastcall*)(uintptr_t,uintptr_t,float,uintptr_t);NodeUpdate node_update_original{};
void NewbornUpdateRow(uintptr_t node,float dt,uint32_t before) noexcept;   // I-0141 tranche 14, defined with ffx_newborn_draw below
std::mutex node_update_lock;uint64_t nu_calls{},nu_no_emitter{},nu_dt_zero{},nu_before{},nu_after{},nu_emitted{};float nu_dt_min{1e9f},nu_dt_max{};
void __fastcall NodeUpdateObserved(uintptr_t node,uintptr_t pass,float dt,uintptr_t emitter) noexcept {
    uint32_t before{};Read(node+0x70,&before,4);
    {   // the chain above the job body 1ADC100 (no static reference): three return addresses up, for the first calls
        static std::atomic<int> traced{};const auto t=traced.fetch_add(1);if(t<3 || (t>=200000 && t<200003)){void* chain[10]{};const auto depth=RtlCaptureStackBackTrace(1,10,chain,nullptr);
            std::string row="{\"event\":\"ffx_node_update_chain\",\"rvas\":[";
            for(USHORT i=0;i<depth;++i){char cell[24]{};sprintf_s(cell,"%s\"%llX\"",i?",":"",static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(chain[i])-image));row+=cell;}
            Log((row+"]}\n").c_str());}}
    node_update_original(node,pass,dt,emitter);
    NewbornUpdateRow(node,dt,before);
    uint32_t after{};Read(node+0x70,&after,4);
    const std::lock_guard hold(node_update_lock);
    ++nu_calls;if(!emitter)++nu_no_emitter;if(dt==0.f)++nu_dt_zero;nu_dt_min=std::min(nu_dt_min,dt);nu_dt_max=std::max(nu_dt_max,dt);
    if(before)++nu_before;if(after)++nu_after;if(after>before)nu_emitted+=after-before;
    if(nu_calls%3000==0){uint8_t pause{};Read(image+0x496237E,&pause,1);
        char row[300]{};sprintf_s(row,"{\"event\":\"ffx_node_update\",\"calls\":%llu,\"no_emitter\":%llu,\"dt_zero\":%llu,\"dt\":[%g,%g],\"with_particles_before\":%llu,\"after\":%llu,\"emitted\":%llu,\"pause\":%u}\n",
            static_cast<unsigned long long>(nu_calls),static_cast<unsigned long long>(nu_no_emitter),static_cast<unsigned long long>(nu_dt_zero),nu_dt_min,nu_dt_max,
            static_cast<unsigned long long>(nu_before),static_cast<unsigned long long>(nu_after),static_cast<unsigned long long>(nu_emitted),pause);Log(row);
        nu_no_emitter=nu_dt_zero=nu_before=nu_after=nu_emitted=0;nu_dt_min=1e9f;nu_dt_max=0.f;}}
// The FFX textures natively (FFX-DRAW §2, Native1942/1944): GXFfxGraphicsResourceManager slot 7 (28F7B0 → 2B7110) finds the
// texture "s%05d" by name in the handler [rm+0x158]'s repositories (+0x10 then +8, the "_a" name first for kinds 0 and 1); the entry
// (SprjGxTexture) forwards to its GXTexture2D [entry+0x10], whose vt+0x10 (2AA60) returns its old view [+0x48], null natively. The
// renderer constructors (381DE0) refuse a null first texture. Native transcription with that last step replaced: the GXTexture2D
// must be one TextureIngress owns, and the renderer receives the native token naming it (effect_draw::TextureToken: 'NFXT',
// its payload the GXTexture2D) that the draw capture resolves through TextureIngress. One token per GXTexture2D, kept for the process.
// FFX-DRAW N6: the GX shader object's handle getter 6D570 ([+0x10], null natively), inside an effect_draw::ProgramScope only
// (the shader set build): a registered object answers with its programme token (effect_draw.h).
// The six getters, slot 2 of GXVertexShader 6D570, GXPixelShader 6D560, GXHullShader 6D550, GXDomainShader 6D530, GXGeometryShader
// 6D540, GXComputeShader 6D520 (E07).
using ShaderHandleGet=uintptr_t(__fastcall*)(uintptr_t);std::array<ShaderHandleGet,6> shader_handle_original{};
constexpr std::array<uint32_t,6> shader_handle_rvas{0x6D570,0x6D560,0x6D550,0x6D530,0x6D540,0x6D520};
// E07 witness (ffx-record.flag): the callers of a getter outside the programme scope that would get a token (a registered object
// with a null handle), by return address, logged at the 1st, 10th and every 1000th such call.
std::mutex outside_lock;std::map<uintptr_t,uint64_t> outside_callers;uint64_t outside_calls{};
template<size_t I> uintptr_t __fastcall ShaderHandle(uintptr_t object) noexcept {
    const auto handle=shader_handle_original[I](object);
    if(handle)return handle;
    static const bool outside_witness=ModuleFlag(L"ffx-record.flag");
    if(!outside_witness && !effect_draw::ProgramScopeOpen())return handle;   // review: no lock in normal play outside the scope
    {const std::lock_guard hold(shader_lock);if(!shaders.count(object))return handle;}
    if(!effect_draw::ProgramScopeOpen()){
        {const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;std::string row;
            {const std::lock_guard hold(outside_lock);++outside_callers[caller*8+I];const auto n=++outside_calls;
                if(n==1 || n==10 || n%1000==0){row="{\"event\":\"native_shader_handle_outside\",\"calls\":"+std::to_string(n)+",\"callers\":{";bool first=true;
                    for(const auto& [k,v]:outside_callers){char c[48]{};sprintf_s(c,"%s\"%llX/%llu\":%llu",first?"":",",static_cast<unsigned long long>(k/8),static_cast<unsigned long long>(k%8),static_cast<unsigned long long>(v));row+=c;first=false;}
                    row+="}}\n";}}
            if(!row.empty())Log(row.c_str());}
        return handle;}
    try{return effect_draw::ProgramToken(object);}catch(...){return handle;}}
std::atomic<uint64_t> native_lookups{},native_found{},native_unowned{};
// I-0141: every texture lookup of the session by (id, kind): calls and non-null answers, both modes (28F7B0 natively NativeTexture,
// in the Reference TextureLookupObserved), written whole on an F9 press (ffx_texture_census): the ids the original answers null.
std::mutex texture_census_lock;std::map<uint64_t,std::pair<uint32_t,uint32_t>> texture_census;
void TextureCensusCount(int32_t id,uint32_t kind,bool found) noexcept {static const bool census=ModuleFlag(L"ffx-record.flag") || ModuleFlag(L"ffx-draw-observe.flag");   // never in play (step review M1)
    if(!id || !census)return;try{const std::lock_guard hold(texture_census_lock);auto& c=texture_census[(uint64_t(uint32_t(id))<<32)|kind];++c.first;if(found)++c.second;}catch(...){}}
void TextureCensusRow(const char* mode) noexcept {try{std::string row="{\"event\":\"ffx_texture_census\",\"mode\":\"";row+=mode;row+="\",\"ids\":[";bool first=true;char c[64]{};
    const std::lock_guard hold(texture_census_lock);for(const auto& [k,v]:texture_census){sprintf_s(c,"%s[%u,%u,%u,%u]",first?"":",",unsigned(k>>32),unsigned(k&0xFFFFFFFFu),v.first,v.second);row+=c;first=false;}
    row+="]}\n";Log(row.c_str());}catch(...){}}
// Witness (ffx-record.flag): the FFX texture id each GXTexture2D was found for (the per-draw dump names textures by it).
std::mutex ffx_texture_id_lock;std::unordered_map<uintptr_t,int32_t> ffx_texture_ids;
uintptr_t __fastcall NativeTexture(uintptr_t rm,int32_t id,uint32_t kind) noexcept {
    if(!id)return 0;
    uintptr_t handler{};if(!Read(rm+0x158,&handler,8) || !handler){TextureCensusCount(id,kind,false);return 0;}   // step review M2
    uintptr_t primary{},second{};Read(handler+8,&primary,8);Read(handler+0x10,&second,8);
    wchar_t plain[16]{},alpha[24]{};swprintf_s(plain,L"s%05d",id);
    if(kind==0 || kind==1)swprintf_s(alpha,L"%s_a",plain);else wcscpy_s(alpha,plain);
    const auto find=[](uintptr_t repo,const wchar_t* name)->uintptr_t{
        uintptr_t table{},method{};if(!repo || !Read(repo,&table,8) || !Read(table+8,&method,8) || !method)return 0;
        return reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const wchar_t*)>(method)(repo,name);};
    uintptr_t entry=second?find(second+8,alpha):0;
    if(!entry)entry=find(primary,alpha);
    if(!entry && second)entry=find(second+8,plain);
    if(!entry)entry=find(primary,plain);
    const auto n=++native_lookups;uintptr_t inner{},inner_vt{},token{};
    if(entry && Read(entry+0x10,&inner,8) && inner && Read(inner,&inner_vt,8) && inner_vt==image+0x3D1D528 && textures.Acquire(inner))token=effect_draw::TextureToken(inner);
    if(token)++native_found;else if(entry)++native_unowned;
    TextureCensusCount(id,kind,token!=0);
    static const bool id_witness=ModuleFlag(L"ffx-record.flag");if(token && id_witness){const std::lock_guard hold(ffx_texture_id_lock);ffx_texture_ids[inner]=id*4+int32_t(kind&3);}
    if(n<=8 || n%1000==0){char row[240]{};sprintf_s(row,"{\"event\":\"native_ffx_texture\",\"call\":%llu,\"id\":%d,\"kind\":%u,\"entry\":%s,\"token\":%s,\"found\":%llu,\"entry_unowned\":%llu}\n",
        static_cast<unsigned long long>(n),id,kind,entry?"true":"false",token?"true":"false",static_cast<unsigned long long>(native_found.load()),
        static_cast<unsigned long long>(native_unowned.load()));Log(row);}
    return token;}
// Witness: the effect resource manager's texture lookup 28F7B0 (id, kind; the texture named "s%05d" or "s%05d_a" in [rm+0x158]'s
// repositories): the first 24 calls and every 500th with the result, then counts of found and null.
using TextureLookup=uintptr_t(__fastcall*)(uintptr_t,int32_t,uint32_t);TextureLookup texture_lookup_original{};std::atomic<uint64_t> lookups{},lookups_null{};
uintptr_t __fastcall TextureLookupObserved(uintptr_t rm,int32_t id,uint32_t kind) noexcept {
    const auto result=texture_lookup_original(rm,id,kind);const auto n=++lookups;if(!result)++lookups_null;TextureCensusCount(id,kind,result!=0);
    if(id && (n<=6 || n%250==1)){   // 2B7110's steps redone: the handler [rm+0x158], its repositories +8/+0x10, the entry each name finds, its vt+0x10
        const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));uintptr_t handler{},repo{},second{},repo_vt{};
        Read(rm+0x158,&handler,8);if(handler){Read(handler+8,&repo,8);Read(handler+0x10,&second,8);}if(repo)Read(repo,&repo_vt,8);
        wchar_t plain[16]{},alpha[20]{};swprintf_s(plain,L"s%05d",id);swprintf_s(alpha,L"%s_a",plain);
        uintptr_t entry[2]{},entry_vt[2]{},texture[2]{};const wchar_t* names[2]{plain,alpha};
        for(int i=0;i<2 && repo_vt;++i){
            entry[i]=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,const wchar_t*)>(*reinterpret_cast<const uintptr_t*>(repo_vt+8))(repo,names[i]);
            if(entry[i] && Read(entry[i],&entry_vt[i],8))texture[i]=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(*reinterpret_cast<const uintptr_t*>(entry_vt[i]+0x10))(entry[i]);}
        // The entry (SprjGxTexture) forwards vt+0x10 to its inner object [entry+0x10]: that object, its vt+0x10, what it returns.
        uintptr_t inner{},inner_vt{},inner_get{},texture_vt{};
        if(entry[0] && Read(entry[0]+0x10,&inner,8) && inner && Read(inner,&inner_vt,8))Read(inner_vt+0x10,&inner_get,8);
        if(texture[0])Read(texture[0],&texture_vt,8);
        char row[480]{};sprintf_s(row,"{\"event\":\"ffx_texture_lookup_steps\",\"call\":%llu,\"id\":%d,\"kind\":%u,\"handler\":%s,\"repo_vt\":\"%llX\",\"second\":%s,"
            "\"plain\":{\"entry\":%s,\"entry_vt\":\"%llX\",\"inner\":%s,\"inner_vt\":\"%llX\",\"inner_get\":\"%llX\",\"texture\":%s,\"texture_vt\":\"%llX\"},\"alpha_entry\":%s}\n",
            static_cast<unsigned long long>(n),id,kind,handler?"true":"false",static_cast<unsigned long long>(repo_vt?repo_vt-base:0),second?"true":"false",
            entry[0]?"true":"false",static_cast<unsigned long long>(entry_vt[0]?entry_vt[0]-base:0),inner?"true":"false",
            static_cast<unsigned long long>(inner_vt?inner_vt-base:0),static_cast<unsigned long long>(inner_get?inner_get-base:0),
            texture[0]?"true":"false",static_cast<unsigned long long>(texture_vt?texture_vt-base:0),entry[1]?"true":"false");Log(row);}
    if(n<=24 || n%500==0){char row[200]{};sprintf_s(row,"{\"event\":\"ffx_texture_lookup\",\"call\":%llu,\"id\":%d,\"kind\":%u,\"found\":%s,\"null_so_far\":%llu}\n",
        static_cast<unsigned long long>(n),id,kind,result?"true":"false",static_cast<unsigned long long>(lookups_null.load()));Log(row);}
    return result;}
std::map<std::pair<uintptr_t,uintptr_t>,uint64_t> entity_slots;std::array<uint64_t,6> node_counts{};std::map<std::string,uint64_t> node_kinds;   // witness: active 1AD6440 entities, nodes, +0x70, +0x58, +0x68, all three
   // witness: the listed entities' vtable and vt+0x38 (engine thread)
// Witness: the draw pass's slot 4 (2997E0, one call per entity the library offers) on the native context, counted.
// 2997E0 returns AL=1 on every exit (its disassembly; Ghidra's void hid it: I-0141 step review M1).
using EntitySlot=uint8_t(__fastcall*)(uintptr_t,uintptr_t,int32_t,uintptr_t,float);EntitySlot entity_original{};uint64_t entity_calls{};
// I-0141 (the native draws effects the original does not): after F9 (Slot27Observed arms entity_rows), both modes (the Reference
// hooks 2997E0 too under ffx-draw-observe.flag), one row an entity offered on pass 0x45's context: its kind, its world position
// (25F9D0 on its vt+0x28 object's +0x60, as 29AA10 reads it), the three masks 2997E0 tests (the resource's +0x1F8/+0x1FC/+0x200
// against the scene control's +0x14C and the pass context's +0x4B8/+0x4BC), 29AA10's answer from the pass camera (1: culled by
// its draw distances) and whether the call recorded a command (the pass context's sort list count [+0x4B0]+0x7C, or the native list).
std::atomic<int> entity_rows{};
// I-0141 (the native spawns cutscene effects the original does not): the scene control's effect creation 24ED90 (sc, handle, FXR id,
// matrix, flag, pre-roll), both modes (ffx-record.flag natively, ffx-draw-observe.flag in the Reference): one row a call, at most
// 6000 a session (ffx_create): the FXR id, the matrix's translation row (+0x30..+0x38), the tick and the callers above the
// 24EFE0/24F0F0 wrappers (RtlCaptureStackBackTrace, module RVAs).
using CreateEffectFn=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uintptr_t,int32_t,int32_t);CreateEffectFn create_effect_original{};std::atomic<int> create_rows{6000};
uintptr_t __fastcall CreateEffectObserved(uintptr_t sc,uintptr_t handle,uint32_t fxr,uintptr_t matrix,int32_t flag,int32_t preroll) noexcept {
    if(create_rows.load()>0 && create_rows.fetch_sub(1)>0){float t[3]{};if(matrix)Read(matrix+0x30,t,12);for(auto& x:t)if(!std::isfinite(x))x=0;
        void* stack[6]{};const USHORT got=RtlCaptureStackBackTrace(1,6,stack,nullptr);LARGE_INTEGER qpc{},rate{};QueryPerformanceCounter(&qpc);QueryPerformanceFrequency(&rate);
        char row[400]{};int at=sprintf_s(row,"{\"event\":\"ffx_create\",\"mode\":\"%s\",\"ms\":%.3f,\"fxr\":%u,\"t\":[%.3f,%.3f,%.3f],\"flag\":%d,\"preroll\":%d,\"callers\":[",
            native_ffx_draw?"native":"reference",1000.0*double(qpc.QuadPart)/double(rate.QuadPart),fxr,t[0],t[1],t[2],flag,preroll);
        for(USHORT i=0;i<got && at>0 && at<360;++i){const auto f=reinterpret_cast<uintptr_t>(stack[i]);at+=sprintf_s(row+at,sizeof(row)-at,"%s\"%llX\"",i?",":"",static_cast<unsigned long long>(f>image && f<image+0x5000000?f-image:0));}
        if(at>0 && at<390){sprintf_s(row+at,sizeof(row)-at,"]}\n");Log(row);}}
    return create_effect_original(sc,handle,fxr,matrix,flag,preroll);}
// I-0141 (« The instances created alike »: the extra entities likely the library's own spawns): the library's instance creation
// 141ACCA20 (the library, FXR id, a3, a4, a 0x30-byte matrix or null, a pre-roll count; the instance returned, null when refused;
// its arguments read in its disassembly, GhidraMCP 2026-11-17: the four homes at [RSP+0x1E0..+0x1F8], the fifth (qword) at
// [RSP+0x200] and the sixth (dword) at [RSP+0x208] after a 0x1C8 frame and two pushes), both modes: one row a call, at most 8000 a
// session (ffx_spawn: the tick, the FXR id, a3, the pre-roll, whether an instance came back, the caller and two frames above it,
// module RVAs), and a tally by caller and FXR id since the start, written at each F9 or F13 press (ffx_spawn_tally).
using SpawnFn=uintptr_t(__fastcall*)(uintptr_t,uint32_t,int32_t,uintptr_t,uintptr_t,uint32_t);SpawnFn spawn_original{};std::atomic<int> spawn_rows{8000};
std::mutex spawn_tally_lock;std::map<std::pair<uint64_t,uint32_t>,std::pair<uint64_t,uint64_t>> spawn_tally;
// I-0141 tranche 12 (the ceremony's smoke sampled at the same moment in both modes, then the entities' age): the scene's own clock,
// started by the first creation of FXR 732035 (the ceremony scene's own effect) through 141ACCA20, both modes (ffx_scene_clock).
std::atomic<int64_t> scene_clock_from{-1},scene_pre_from{-1};
double QpcMs() noexcept {LARGE_INTEGER now{},rate{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&rate);return double(now.QuadPart)*1000.0/double(rate.QuadPart);}
double SceneMs() noexcept {const int64_t from=scene_clock_from.load(std::memory_order_relaxed);if(from<0)return -1.0;
    LARGE_INTEGER now{},rate{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&rate);return double(now.QuadPart-from)*1000.0/double(rate.QuadPart);}
// ffx-entity-arm.txt beside the module (one number, ms on the scene clock): the light press (F13's rows) once at that moment.
bool ScenePressDue() noexcept {
    static const double at=[]{const auto path=ModuleSibling(L"ffx-entity-arm.txt");double v=-1.0;FILE* file{};
        if(!path.empty() && _wfopen_s(&file,path.c_str(),L"rb")==0 && file){if(fscanf_s(file,"%lf",&v)!=1 || !std::isfinite(v) || v<0)v=-1.0;fclose(file);}return v;}();
    static std::atomic<bool> done{};if(at<0 || done.load(std::memory_order_relaxed))return false;
    const double ms=SceneMs();if(ms<0 || ms<at || done.exchange(true))return false;
    char row[128]{};sprintf_s(row,"{\"event\":\"ffx_scene_press\",\"mode\":\"%s\",\"scene_ms\":%.1f,\"at\":%.1f}\n",native_ffx_draw?"native":"reference",ms,at);Log(row);return true;}
// I-0141 tranche 12: the effect instances' stops (DarkSoulsIII 1.15.2, READ): 141AB18C0 sets the instance's +0x80 bit 4, the hard
// stop: the next library update (141AB1680) deletes every object of the instance through 141AB1170, particles left or not;
// 141AB1080 sets bit 8 (called by the handle's 141A87C60 with 141AB1140 clearing bit 2, the soft stop, and by 2695E0 and its twin
// at 277F43 beside their matrix stores, not shown to be a stop: rows "bit8", told apart by their callers). With
// ffx-entity-life.flag, both modes, from the scene clock's start for 90 s, one row a call (ffx_instance_flag): the instance's FXR id
// from its creation (141ACCA20, 0 when unseen), its flags before the call, the scene time and three callers. At most 4000 rows.
using InstanceFlagFn=void(__fastcall*)(uintptr_t);InstanceFlagFn stop_hard_original{},stop_soft_original{};
std::mutex instance_fxr_lock;std::unordered_map<uintptr_t,uint32_t> instance_fxr;std::atomic<int> instance_flag_rows{4000};
double InstanceFlagDue(uintptr_t instance) noexcept {   // the scene time when a row is due, -1 otherwise (the stack walked only then)
    static const bool on=ModuleFlag(L"ffx-entity-life.flag");if(!on || !instance)return -1.0;
    const double ms=SceneMs();if(ms<0 || ms>90000.0)return -1.0;
    if(instance_flag_rows.load(std::memory_order_relaxed)<=0 || instance_flag_rows.fetch_sub(1)<=0)return -1.0;
    return ms;}
void InstanceFlagRow(uintptr_t instance,const char* what,double ms,const uint64_t (&at)[3]) noexcept {
    uint32_t fxr{};{const std::lock_guard hold(instance_fxr_lock);const auto f=instance_fxr.find(instance);if(f!=instance_fxr.end())fxr=f->second;}
    uint32_t flags{};const bool read=Read(instance+0x80,&flags,4);char shown[16]{};if(read)sprintf_s(shown,"\"%X\"",flags);else strcpy_s(shown,"null");
    char row[320]{};sprintf_s(row,"{\"event\":\"ffx_instance_flag\",\"mode\":\"%s\",\"what\":\"%s\",\"scene_ms\":%.1f,\"instance\":\"%llX\",\"fxr\":%u,\"flags\":%s,\"callers\":[\"%llX\",\"%llX\",\"%llX\"]}\n",
        native_ffx_draw?"native":"reference",what,ms,static_cast<unsigned long long>(instance),fxr,shown,
        static_cast<unsigned long long>(at[0]),static_cast<unsigned long long>(at[1]),static_cast<unsigned long long>(at[2]));Log(row);}
__declspec(noinline) void CallerRvas(uint64_t (&at)[3]) noexcept {   // called by the hook itself: frame 0 is the game's caller of the hooked function
    void* stack[4]{};const USHORT got=RtlCaptureStackBackTrace(2,3,stack,nullptr);
    for(USHORT i=0;i<got && i<3;++i){const auto f=reinterpret_cast<uintptr_t>(stack[i]);at[i]=f>image && f<image+0x5000000?f-image:0;}}
void __fastcall StopHardObserved(uintptr_t instance) noexcept {if(const double ms=InstanceFlagDue(instance);ms>=0){uint64_t at[3]{};CallerRvas(at);InstanceFlagRow(instance,"stop",ms,at);}stop_hard_original(instance);}
void __fastcall StopSoftObserved(uintptr_t instance) noexcept {if(const double ms=InstanceFlagDue(instance);ms>=0){uint64_t at[3]{};CallerRvas(at);InstanceFlagRow(instance,"bit8",ms,at);}stop_soft_original(instance);}
// I-0141 tranche 13: the type-0x898 effect object's state 3 (DarkSoulsIII 1.15.2, READ): 1AD71A0 runs it each update and leaves it
// for 1AD6740 (the end, state 8) when (a) the duration 1AD6520 evaluates from the state's FXR property (+inf when negative) is <= the
// age (+0xA8), or (b) the spawner +0xC0 is null or its vt+0x18 answers false, or (c) the flag 0x400 is set (+0xB0) and either the
// flag 2 is set or an end is asked (the instance +0x10's +0x80 bit 3, or the FXR state machine's index +0x90+4 out of its count)
// while the +0x18 object is null or of type 2000 or 2001; the FXR state machine moving to another state leaves through 1AD6BB0 or
// 1AD65F0. With ffx-entity-life.flag, both modes, from the first ceremony FXR's creation (ffx_scene_pre) to 45 s of the scene clock: per object, its first and last call, the
// age and the duration at both and the duration's range, the flags, the instance's flags, the state machine's index and count,
// +0xB8, whether +0x18 and +0xC0 are set (their vtables), and the state returned; one row (ffx_state3) when the state returned is
// not 3 (« exit »), and at the close one row for each object still in state 3 at its last call (« open »). At most 20000 rows.
using State3Fn=uint64_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);State3Fn state3_original{};
using DurationFn=float(__fastcall*)(uintptr_t,uintptr_t);DurationFn duration_original{};
thread_local uintptr_t duration_for{};thread_local float duration_seen{};thread_local bool duration_read{};
float __fastcall DurationObserved(uintptr_t object,uintptr_t property) noexcept {
    const float v=duration_original(object,property);if(object && object==duration_for){duration_seen=v;duration_read=true;}return v;}
struct State3Inputs {float age{};uint32_t flags{},instance_flags{},index{},count{},b8{};uintptr_t o18{},oc0{},instance{};bool ok{},instance_read{},machine_read{};};
State3Inputs ReadState3(uintptr_t object) noexcept {
    State3Inputs s{};s.ok=Read(object+0xA8,&s.age,4) && Read(object+0xB0,&s.flags,4) && Read(object+0xB8,&s.b8,4) && Read(object+0x18,&s.o18,8) && Read(object+0xC0,&s.oc0,8);
    uintptr_t instance{},machine{},table{};
    if(Read(object+0x10,&instance,8) && instance){s.instance=instance;s.instance_read=Read(instance+0x80,&s.instance_flags,4);}
    if(Read(object+0x90,&machine,8) && machine && Read(machine+4,&s.index,4) && Read(machine+0x10,&table,8) && table)s.machine_read=Read(table+4,&s.count,4);
    return s;}
uint64_t VtableRva(uintptr_t object) noexcept {uintptr_t vt{};if(!object || !Read(object,&vt,8))return 0;return vt>image && vt<image+0x5000000?vt-image:0;}
struct State3Life {double first{},last{};uint64_t calls{};State3Inputs at_first{},at_last{};float duration_first{},duration_last{},duration_min{},duration_max{};
    bool duration_any{};uint64_t vt18{},vtc0{},vtc0_last{};uint64_t result{3};uint32_t moves{},fxr{};bool moved_now{};};
std::mutex state3_lock;std::unordered_map<uintptr_t,State3Life> state3_lives;std::atomic<int> state3_rows{20000},state3_dropped{};std::atomic<bool> state3_closed{};
void State3Row(uintptr_t object,const State3Life& l,const char* what) noexcept {
    // Called under state3_lock: the cap and the rows it refused are counted exactly (the step review of ab6145dcc, M2). The FXR id is
    // the one looked up while the object lived (its instance read in its calls), never read again here: an object deleted at the cut
    // is written « open » at the close from freed memory (the step review of ab6145dcc, M1).
    if(state3_rows.load(std::memory_order_relaxed)<=0 || state3_rows.fetch_sub(1)<=0){++state3_dropped;return;}
    const uint32_t fxr=l.fxr;
    const auto num=[](float v,char (&out)[24]){if(std::isfinite(v))sprintf_s(out,"%.6g",double(v));else if(std::isnan(v))strcpy_s(out,"\"nan\"");else strcpy_s(out,v>0?"\"inf\"":"\"-inf\"");};
    char a0[24]{},a1[24]{},d0[24]{},d1[24]{},dmin[24]{},dmax[24]{};num(l.at_first.age,a0);num(l.at_last.age,a1);
    if(l.duration_any){num(l.duration_first,d0);num(l.duration_last,d1);num(l.duration_min,dmin);num(l.duration_max,dmax);}
    else{strcpy_s(d0,"null");strcpy_s(d1,"null");strcpy_s(dmin,"null");strcpy_s(dmax,"null");}
    const auto& f=l.at_first;const auto& t=l.at_last;char row[900]{};
    sprintf_s(row,"{\"event\":\"ffx_state3\",\"mode\":\"%s\",\"what\":\"%s\",\"object\":\"%llX\",\"fxr\":%u,\"first\":%.1f,\"last\":%.1f,\"calls\":%llu,\"result\":%llu,"
        "\"age\":[%s,%s],\"duration\":[%s,%s,%s,%s],\"flags\":[\"%X\",\"%X\"],\"instance_flags\":[%s,%s],\"machine\":[%d,%u,%u,%u,%u],\"b8\":[%u,%u],"
        "\"o18\":[%d,%d,\"%llX\"],\"oc0\":[%d,%d,\"%llX\",\"%llX\"],\"moves\":[%u,%d],\"ok\":[%d,%d]}\n",
        native_ffx_draw?"native":"reference",what,static_cast<unsigned long long>(object),fxr,l.first,l.last,static_cast<unsigned long long>(l.calls),static_cast<unsigned long long>(l.result),
        a0,a1,d0,d1,dmin,dmax,f.flags,t.flags,f.instance_read?std::to_string(f.instance_flags).c_str():"null",t.instance_read?std::to_string(t.instance_flags).c_str():"null",
        int(t.machine_read),f.index,t.index,f.count,t.count,f.b8,t.b8,
        int(f.o18!=0),int(t.o18!=0),static_cast<unsigned long long>(l.vt18),int(f.oc0!=0),int(t.oc0!=0),static_cast<unsigned long long>(l.vtc0),static_cast<unsigned long long>(l.vtc0_last),
        l.moves,int(l.moved_now),int(f.ok),int(t.ok));
    Log(row);}
uint64_t __fastcall State3Observed(uintptr_t object,uintptr_t a2,uintptr_t a3) noexcept {
    static const bool on=ModuleFlag(L"ffx-entity-life.flag");
    if(!on || !object || state3_closed.load(std::memory_order_relaxed))return state3_original(object,a2,a3);
    if(scene_pre_from.load(std::memory_order_relaxed)<0)return state3_original(object,a2,a3);
    const double ms=QpcMs();   // QPC milliseconds, as ffx_scene_pre's and ffx_scene_clock's "ms": the reader puts them on the scene clock
    // A move of the FXR state machine (index +0x90+4 other than +0xB8) inside the call: 1AD6BB0 resets the age and the spawner and
    // sets 0x400 before the exit test, or 1AD65F0 → 1AD75E0 calls 1AD71A0 again on the same object and thread (the step review of
    // ed1e92f74, H1). The innermost call records; an outer call whose inner call recorded only passes its result on. A call that
    // stays in state 3 keeps the inputs read after it (the age reset), so the next call's age does not look like a reused address.
    static thread_local int depth{};static thread_local bool inner_recorded{};
    if(depth>0)inner_recorded=true;else inner_recorded=false;
    const State3Inputs before=ReadState3(object);const bool moving=before.machine_read && before.index!=before.b8;
    const uintptr_t outer_for=duration_for;duration_for=object;duration_read=false;
    ++depth;const bool was_inner=depth>1;const uint64_t result=state3_original(object,a2,a3);--depth;
    const bool have_duration=duration_read;const float duration=duration_seen;duration_for=outer_for;
    if(!was_inner && inner_recorded){inner_recorded=false;return result;}
    const State3Inputs after=result==3?ReadState3(object):before;
    const std::lock_guard hold(state3_lock);if(state3_closed.load())return result;
    // Closed at 45 s of the scene clock (the cut at about 30 s; the runs reach about 67 s, the final review of 20d70b554's M6).
    // The end row: when it closed (QPC ms, read now), the lives still open then, the rows left and the rows the cap refused.
    if(SceneMs()>45000.0){state3_closed=true;int open{};for(const auto& [o,l]:state3_lives)if(l.result==3){++open;State3Row(o,l,"open");}state3_lives.clear();
        char row[192]{};sprintf_s(row,"{\"event\":\"ffx_state3_end\",\"mode\":\"%s\",\"ms\":%.1f,\"open\":%d,\"rows_left\":%d,\"dropped\":%d}\n",native_ffx_draw?"native":"reference",QpcMs(),open,state3_rows.load(),state3_dropped.load());Log(row);return result;}
    auto found=state3_lives.find(object);
    // An object freed and another born at its address: its age goes back (the ages only grow within a life but at a move).
    if(found!=state3_lives.end() && !moving && before.ok && before.age+0.001f<found->second.at_last.age){if(found->second.result==3)State3Row(found->first,found->second,"open");state3_lives.erase(found);found=state3_lives.end();}
    if(found==state3_lives.end()){found=state3_lives.emplace(object,State3Life{}).first;found->second.first=ms;found->second.at_first=before;}
    auto& l=found->second;
    if(before.instance){const std::lock_guard hold_fxr(instance_fxr_lock);const auto f=instance_fxr.find(before.instance);if(f!=instance_fxr.end())l.fxr=f->second;}
    l.last=ms;++l.calls;l.at_last=result==3?after:before;l.result=result;l.moved_now=moving;if(moving)++l.moves;
    if(!l.vt18 && before.o18)l.vt18=VtableRva(before.o18);if(!l.vtc0 && before.oc0)l.vtc0=VtableRva(before.oc0);
    if(const uintptr_t oc0=result==3?after.oc0:before.oc0)l.vtc0_last=VtableRva(oc0);
    if(have_duration){if(!l.duration_any){l.duration_any=true;l.duration_first=duration;l.duration_min=duration;l.duration_max=duration;}
        l.duration_last=duration;if(duration<l.duration_min)l.duration_min=duration;if(duration>l.duration_max)l.duration_max=duration;}
    if(result!=3){State3Row(object,l,"exit");state3_lives.erase(found);}
    return result;}
// I-0141 tranche 13b: the FXR state machine (DarkSoulsIII 1.15.2, READ): FfxLibraryUpdateInstance 141AB1680 steps each machine of the
// instance's +0x78 list through 141ACA640 (machine, instance, dt): the state time +8 += dt; while the index +4 is below the count
// (+0x10→+4), each condition of the state (+0x10→+8 + index*0x10: count +4, conditions +8, stride 0x60) is evaluated by 141ACA8A0:
// operator u16 +0 (0-3 int, 4-7 uint, 8-11 float: ==, !=, <, <=), next index u32 +8, operands at +0x10 and +0x38, each evaluated
// by 141ACA310 (operand, out, state time, instance) from its kind u16: 0xFFFC a constant (*[+0x10]), 0xFFFD the instance's external
// value (+0x48 table: keys +8, values +0x10, count +0x18; key *[+0x10]), 0xFFFE a camera distance, 0xFFFF the state time. The first
// true condition sets the index (an index at or past the count ends the type-0x898 objects, 1AD71A0). In the original the ceremony's
// objects leave state 3 by the index -1 at 14.4 s and at the cut, natively never (tranche 13's rows). With ffx-entity-life.flag, both
// modes, the ceremony window (ffx_scene_pre to 45 s), for the instances of FXR 732000..732999 and 4040: one row (ffx_machine) per
// machine when first stepped, at each index change and every 5 s of its life: the index before and after, the state time, dt, the
// state's conditions (operator, next, both operands' kinds, constants or keys and their values evaluated by 141ACA310) and the
// instance's external values. At most 6000 rows.
using MachineFn=void(__fastcall*)(uintptr_t,uintptr_t,float);MachineFn machine_original{};
// The state time goes in XMM2 (a float: 141ACA8A0 loads it with MOVSS XMM2 before each call; the step review of 99e241966, H1).
using OperandFn=void(__fastcall*)(const void*,uint32_t*,float,uintptr_t);
std::mutex machine_lock;struct MachineSeen {double ms{};uintptr_t instance{};float time{};};std::unordered_map<uintptr_t,MachineSeen> machine_seen;std::atomic<int> machine_rows{6000};
void MachineRow(uintptr_t machine,uintptr_t instance,uint32_t fxr,float dt,uint32_t before,uint32_t after,float time,double ms,const char* why) noexcept {
    if(machine_rows.load(std::memory_order_relaxed)<=0 || machine_rows.fetch_sub(1)<=0)return;
    std::string row;char cell[384]{};const auto fin=[](float v){return std::isfinite(v)?double(v):0.0;};
    sprintf_s(cell,"{\"event\":\"ffx_machine\",\"mode\":\"%s\",\"why\":\"%s\",\"ms\":%.1f,\"fxr\":%u,\"instance\":\"%llX\",\"machine\":\"%llX\",\"index\":[%d,%d],\"time\":%.6g,\"dt\":%.6g",
        native_ffx_draw?"native":"reference",why,ms,fxr,static_cast<unsigned long long>(instance),static_cast<unsigned long long>(machine),int(before),int(after),fin(time),fin(dt));row+=cell;
    uintptr_t table{},states{};uint32_t count{};
    if(Read(machine+0x10,&table,8) && table && Read(table+4,&count,4) && Read(table+8,&states,8) && states){
        uintptr_t conditions{};uint32_t n{};
        const bool state_read=before<count && Read(states+uintptr_t(before)*0x10+4,&n,4) && Read(states+uintptr_t(before)*0x10+8,&conditions,8) && conditions;
        sprintf_s(cell,",\"count\":%u,\"n\":%d,\"conditions\":[",count,state_read?int(n):-1);row+=cell;   // n: the state's condition count (six shown at most)
        if(state_read){
            const auto operand=reinterpret_cast<OperandFn>(image+0x1ACA310);
            for(uint32_t i=0;i<n && i<6;++i){const uintptr_t c=conditions+uintptr_t(i)*0x60;uint16_t op{},lk{},rk{};uint32_t next{};uintptr_t lp{},rp{};uint32_t lkey{},rkey{};
                if(!Read(c,&op,2) || !Read(c+8,&next,4) || !Read(c+0x10,&lk,2) || !Read(c+0x38,&rk,2))break;
                if(Read(c+0x20,&lp,8) && lp)Read(lp,&lkey,4);if(Read(c+0x48,&rp,8) && rp)Read(rp,&rkey,4);
                // The constant (FFFC) and external (FFFD) kinds dereference their value pointer unchecked: not called on a null one (the step
                // review of d6af2dbec, L2; the conditions after a true one were not evaluated by the game at that step).
                const auto safe=[](uint16_t kind,uintptr_t p){return p || (kind!=0xFFFC && kind!=0xFFFD);};
                uint32_t lv{},rv{};if(safe(lk,lp))operand(reinterpret_cast<const void*>(c+0x10),&lv,time,instance);if(safe(rk,rp))operand(reinterpret_cast<const void*>(c+0x38),&rv,time,instance);
                float lf{},rf{};std::memcpy(&lf,&lv,4);std::memcpy(&rf,&rv,4);
                sprintf_s(cell,"%s{\"op\":%u,\"next\":%d,\"left\":[\"%X\",\"%X\",\"%X\",%.6g],\"right\":[\"%X\",\"%X\",\"%X\",%.6g]}",i?",":"",op,int(next),lk,lkey,lv,fin(lf),rk,rkey,rv,fin(rf));row+=cell;}}
        row+="]";}
    uintptr_t ext{},keys{},values{};uint32_t en{};const bool ext_read=Read(instance+0x48,&ext,8) && ext && Read(ext+8,&keys,8) && Read(ext+0x10,&values,8) && Read(ext+0x18,&en,4) && keys && values;
    sprintf_s(cell,",\"external_n\":%d,\"external\":[",ext_read?int(en):-1);row+=cell;   // eight shown at most
    if(ext_read)
        for(uint32_t i=0;i<en && i<8;++i){int32_t k{};uint32_t v{};if(!Read(keys+i*4,&k,4) || !Read(values+i*4,&v,4))break;float f{};std::memcpy(&f,&v,4);
            sprintf_s(cell,"%s[%d,\"%X\",%.6g]",i?",":"",k,v,fin(f));row+=cell;}
    row+="]}\n";Log(row.c_str());}
void __fastcall MachineObserved(uintptr_t machine,uintptr_t instance,float dt) noexcept {
    static const bool on=ModuleFlag(L"ffx-entity-life.flag");
    // The window closes at 45 s of the scene clock even without a 1AD71A0 call (the step review of 99e241966, L3).
    if(!on || !machine || !instance || state3_closed.load(std::memory_order_relaxed) || scene_pre_from.load(std::memory_order_relaxed)<0 || SceneMs()>45000.0){machine_original(machine,instance,dt);return;}
    uint32_t fxr{};{const std::lock_guard hold(instance_fxr_lock);const auto f=instance_fxr.find(instance);if(f!=instance_fxr.end())fxr=f->second;}
    if(!((fxr>=732000 && fxr<=732999) || fxr==4040)){machine_original(machine,instance,dt);return;}
    uint32_t before{};float time{};const bool read=Read(machine+4,&before,4) && Read(machine+8,&time,4);
    machine_original(machine,instance,dt);
    if(!read)return;
    uint32_t after{};if(!Read(machine+4,&after,4))return;
    const double ms=QpcMs();const std::lock_guard hold(machine_lock);
    // A machine freed and another made at its address for another instance is a new one (the step review of 99e241966, M2).
    // ... and a new machine of a reused instance address, told by its state time going back without a move (the step review of d6af2dbec, M2).
    auto [it,fresh]=machine_seen.try_emplace(machine,MachineSeen{ms,instance,time+dt});
    if(!fresh && (it->second.instance!=instance || (after==before && time+dt+0.0001f<it->second.time))){it->second.instance=instance;fresh=true;}
    it->second.time=after!=before?0.0f:time+dt;
    const char* why=fresh?"first":after!=before?"move":ms-it->second.ms>=5000.0?"every":nullptr;
    if(!why)return;
    it->second.ms=ms;MachineRow(machine,instance,fxr,dt,before,after,time+dt,ms,why);}
// I-0141 tranche 13c: who ends the ceremony's effects (tranche 13b, OBSERVED: their machines' one condition is 1 <= the external value
// 0, and the original sets a table {0: 1} on their instances 1 s after 732000's and 732010's creation, at 14.4 s and at the cut,
// natively never). 141AB1A60 (instance, table; READ) puts a table of external values on an instance: copied into its +0x48 table
// when the key counts match, else that table replaced; called by CreateFfxInstance 141ACCA20 and FfxHandle_ApplyExternalValues
// 141A87DF0 (whose callers apply the global stop table at 144773698, 1404D2AC0 among them; 1404D2AC0 and 140870CD0's second path
// reach it by tail jumps through the thunk 140244A50, so they never show in a chain: there frame 1 is their own caller, the step
// review of 4847b46e3's L1). With ffx-entity-life.flag, both modes, the ceremony window, for the instances of FXR 732000..732999 and
// 4040 known from their creation: one row a call (ffx_ext_set): the table's entries (-1 when unreadable), the result and six callers
// (module RVAs); a call made inside a creation (141ACCA20, hooked by SpawnObserved, which names the FXR being created in the
// thread's creating_fxr, before the instance is in the creation map) takes that FXR, marked « creation » when frame 0 is
// 1ACCC05 (the step reviews of 4847b46e3 and 1b44799dc, M1). At most 2000 rows; the close row (ffx_ext_set_end, its time, the rows
// left and the rows the cap refused) is written at the first call after the window closes, its counts a snapshot.
using ExtSetFn=uint64_t(__fastcall*)(uintptr_t,uintptr_t);ExtSetFn ext_set_original{};std::atomic<int> ext_set_rows{2000},ext_set_dropped{};std::atomic<bool> ext_set_closed{};
thread_local uint32_t creating_fxr{};   // the FXR id SpawnObserved is creating on this thread (0: none)
uint64_t __fastcall ExtSetObserved(uintptr_t instance,uintptr_t table) noexcept {
    static const bool on=ModuleFlag(L"ffx-entity-life.flag");
    if(!on || !instance || scene_pre_from.load(std::memory_order_relaxed)<0)return ext_set_original(instance,table);
    if(state3_closed.load(std::memory_order_relaxed) || SceneMs()>45000.0){   // the close row, once (the step review's L3)
        if(!ext_set_closed.exchange(true)){char row[128]{};sprintf_s(row,"{\"event\":\"ffx_ext_set_end\",\"mode\":\"%s\",\"ms\":%.1f,\"rows_left\":%d,\"dropped\":%d}\n",native_ffx_draw?"native":"reference",QpcMs(),(std::max)(0,ext_set_rows.load()),ext_set_dropped.load());Log(row);}
        return ext_set_original(instance,table);}
    // The FXR filter first, the stack walked only for a row kept (the step review of 4847b46e3, L2).
    uint32_t fxr=creating_fxr;
    if(!fxr){const std::lock_guard hold(instance_fxr_lock);const auto f=instance_fxr.find(instance);if(f!=instance_fxr.end())fxr=f->second;}
    if(!((fxr>=732000 && fxr<=732999) || fxr==4040))return ext_set_original(instance,table);
    if(ext_set_rows.load(std::memory_order_relaxed)<=0 || ext_set_rows.fetch_sub(1)<=0){++ext_set_dropped;return ext_set_original(instance,table);}
    void* stack[7]{};const USHORT got=RtlCaptureStackBackTrace(1,6,stack,nullptr);   // this hook's own caller chain
    const uint64_t result=ext_set_original(instance,table);
    const bool creation=creating_fxr && got>0 && reinterpret_cast<uintptr_t>(stack[0])==image+0x1ACCC05;
    std::string row;char cell[256]{};
    sprintf_s(cell,"{\"event\":\"ffx_ext_set\",\"mode\":\"%s\",\"ms\":%.1f,\"fxr\":%u,\"creation\":%d,\"instance\":\"%llX\",\"table\":\"%llX\",\"result\":%d,\"entries\":[",
        native_ffx_draw?"native":"reference",QpcMs(),fxr,int(creation),static_cast<unsigned long long>(instance),static_cast<unsigned long long>(table),int(result&0xFF));row+=cell;
    uintptr_t keys{},values{};uint32_t en{};
    // -1 only when the header is unreadable; an empty table is 0 (the step review of 1b44799dc, L3).
    const bool table_read=table && Read(table+8,&keys,8) && Read(table+0x10,&values,8) && Read(table+0x18,&en,4);
    if(table_read && keys && values)
        for(uint32_t i=0;i<en && i<8;++i){int32_t k{};uint32_t v{};if(!Read(keys+i*4,&k,4) || !Read(values+i*4,&v,4))break;float f{};std::memcpy(&f,&v,4);
            sprintf_s(cell,"%s[%d,\"%X\",%.6g]",i?",":"",k,v,std::isfinite(f)?double(f):0.0);row+=cell;}
    sprintf_s(cell,"],\"entries_n\":%d,\"callers\":[",table_read?int(en):-1);row+=cell;
    for(USHORT i=0;i<got && i<6;++i){const auto f=reinterpret_cast<uintptr_t>(stack[i]);sprintf_s(cell,"%s\"%llX\"",i?",":"",static_cast<unsigned long long>(f>image && f<image+0x5000000?f-image:0));row+=cell;}
    row+="]}\n";Log(row.c_str());return result;}
uintptr_t __fastcall SpawnObserved(uintptr_t library,uint32_t fxr,int32_t a3,uintptr_t a4,uintptr_t matrix,uint32_t preroll) noexcept {
    if(fxr==732035 && scene_clock_from.load(std::memory_order_relaxed)<0){LARGE_INTEGER q{},rate{};QueryPerformanceCounter(&q);QueryPerformanceFrequency(&rate);int64_t expected=-1;
        if(scene_clock_from.compare_exchange_strong(expected,q.QuadPart)){char row[128]{};
            sprintf_s(row,"{\"event\":\"ffx_scene_clock\",\"mode\":\"%s\",\"fxr\":%u,\"ms\":%.3f}\n",native_ffx_draw?"native":"reference",fxr,1000.0*double(q.QuadPart)/double(rate.QuadPart));Log(row);}}
    // I-0141 tranche 13 (the final review of 20d70b554, M2): the scene's effects start about 38.5 s before 732035; the state-3 rows
    // start at the first creation of any of the ceremony's FXR ids 732000..732999 (ffx_scene_pre, the same QPC milliseconds).
    if(fxr>=732000 && fxr<=732999 && scene_pre_from.load(std::memory_order_relaxed)<0){LARGE_INTEGER q{},rate{};QueryPerformanceCounter(&q);QueryPerformanceFrequency(&rate);int64_t expected=-1;
        if(scene_pre_from.compare_exchange_strong(expected,q.QuadPart)){char row[128]{};
            sprintf_s(row,"{\"event\":\"ffx_scene_pre\",\"mode\":\"%s\",\"fxr\":%u,\"ms\":%.3f}\n",native_ffx_draw?"native":"reference",fxr,1000.0*double(q.QuadPart)/double(rate.QuadPart));Log(row);}}
    void* stack[3]{};const USHORT got=RtlCaptureStackBackTrace(1,3,stack,nullptr);uint64_t at_rva[3]{};
    for(USHORT i=0;i<got;++i){const auto f=reinterpret_cast<uintptr_t>(stack[i]);at_rva[i]=f>image && f<image+0x5000000?f-image:0;}
    const uint32_t outer=creating_fxr;creating_fxr=fxr;   // ExtSetObserved's FXR for the calls inside this creation (nesting kept)
    const uintptr_t made=spawn_original(library,fxr,a3,a4,matrix,preroll);
    creating_fxr=outer;
    {const std::lock_guard hold(spawn_tally_lock);auto& t=spawn_tally[{at_rva[0],fxr}];++t.first;if(made)++t.second;}
    static const bool life=ModuleFlag(L"ffx-entity-life.flag");
    if(life && made){const std::lock_guard hold(instance_fxr_lock);if(instance_fxr.size()>200000)instance_fxr.clear();instance_fxr[made]=fxr;}
    if(spawn_rows.load()>0 && spawn_rows.fetch_sub(1)>0){LARGE_INTEGER qpc{},rate{};QueryPerformanceCounter(&qpc);QueryPerformanceFrequency(&rate);
        char row[320]{};sprintf_s(row,"{\"event\":\"ffx_spawn\",\"mode\":\"%s\",\"ms\":%.3f,\"fxr\":%u,\"a3\":%d,\"preroll\":%u,\"matrix\":%d,\"made\":%d,\"callers\":[\"%llX\",\"%llX\",\"%llX\"]}\n",
            native_ffx_draw?"native":"reference",1000.0*double(qpc.QuadPart)/double(rate.QuadPart),fxr,a3,preroll,int(matrix!=0),int(made!=0),
            static_cast<unsigned long long>(at_rva[0]),static_cast<unsigned long long>(at_rva[1]),static_cast<unsigned long long>(at_rva[2]));Log(row);}
    return made;}
void SpawnTallyRows() noexcept {
    std::string row="{\"event\":\"ffx_spawn_tally\",\"mode\":\"";row+=native_ffx_draw?"native":"reference";LARGE_INTEGER qpc{},rate{};QueryPerformanceCounter(&qpc);QueryPerformanceFrequency(&rate);char tick[64]{};sprintf_s(tick,"\",\"ms\":%.3f,\"spawn_rows_left\":%d",1000.0*double(qpc.QuadPart)/double(rate.QuadPart),spawn_rows.load());row+=tick;row+=",\"rows\":[";char cell[96]{};bool first=true;
    {const std::lock_guard hold(spawn_tally_lock);for(const auto& [key,count]:spawn_tally){
        sprintf_s(cell,"%s[\"%llX\",%u,%llu,%llu]",first?"":",",static_cast<unsigned long long>(key.first),key.second,static_cast<unsigned long long>(count.first),static_cast<unsigned long long>(count.second));row+=cell;first=false;}}
    row+="]}\n";Log(row.c_str());}
// I-0141 tranche 12: with ffx-entity-life.flag, every entity offered on pass 0x45 from the scene clock's start for 90 s, both modes: its
// kind, masks (the resource's +0x1F8..+0x200), vtable and world position when first offered, its first and last offer on the scene
// clock and its offers, one row when its life ends (ffx_entity_life): not offered in the last 12 walks of the pass (a walk: offers
// less than 4 ms apart; two a frame natively) and for 100 ms (a hitch alone ends nothing), or offered again with another kind, vtable,
// object or resource, or for an emitter node a new particle array (the address reused, or a head's contents moved: « reborn »); and each second the entities offered in the last 100 ms by kind and first
// mask with the longest gap between two offers since the last count row (ffx_entity_life_count). At most 30000 life rows.
// For the emitter node's entity (vtable 3A9CA60, at the 0xC0-byte node's +0x78; its constructor 1ADB540 is the vtable's only writer)
// each row also carries its node's state ("node"): the particle count (+0x70) at the birth, at the last offer and its maximum, the
// capacity (+0x74), and at the birth and the last offer the bits 1 renderer (+0x58), 2 the +0x98 object, 4 the +0xA0 object,
// 8 the particle array (+0x60). The library frees a non-head node whose count is 0 (1ADAF70) and ends its owner only once the
// counts of the nodes not ended are 0 (1AD6740). A head node's row also carries its owner ("owner"): the owner's state at the
// first offer whose owner read succeeded (not always the birth), and at the last offer its state, age, the age its end counts
// from and its flags.
struct NodeState {uint32_t count{},cap{};uint8_t bits{};bool valid{};uintptr_t particles{};bool head{};int32_t state{};float age{},from{};uint32_t flags{};};
NodeState ReadNodeState(uintptr_t entity,uintptr_t vt) noexcept {
    NodeState s{};if(vt!=image+0x3A9CA60)return s;const uintptr_t node=entity-0x78;uintptr_t p[4]{};
    if(!Read(node+0x70,&s.count,4) || !Read(node+0x74,&s.cap,4) || !Read(node+0x58,&p[0],8) || !Read(node+0x98,&p[1],8) || !Read(node+0xA0,&p[2],8) || !Read(node+0x60,&p[3],8))return NodeState{};
    for(int i=0;i<4;++i)if(p[i])s.bits|=uint8_t(1u<<i);s.particles=p[3];s.valid=true;
    // The head node is the type-0x898 effect object's +0xD0 (1AD4C00): its owner's state (+0xBC, 1AD5D30's switch; 10 ends it), age
    // (+0xA8), flags (+0xB0: bit 4 dead) and the age its end counts from (+0x190, 1AD6740).
    const uintptr_t owner=node-0xD0;uintptr_t ovt{};
    if(Read(owner,&ovt,8) && ovt==image+0x3A9C150 && Read(owner+0xBC,&s.state,4) && Read(owner+0xA8,&s.age,4) && Read(owner+0xB0,&s.flags,4) && Read(owner+0x190,&s.from,4))s.head=true;
    return s;}
struct EntityLife {double first{},last{};uint64_t offers{},last_walk{};int32_t kind{};uint32_t masks[3]{};float position[3]{};uintptr_t vt{},object{},resource{};
    bool node{};uint32_t count_birth{},count_last{},count_max{},cap{};uint8_t bits_birth{},bits_last{};uintptr_t particles{};
    bool head{};int32_t state_birth{},state_last{};float age_last{},from_last{};uint32_t flags_last{};};
std::mutex life_lock;std::map<uintptr_t,EntityLife> lives;double life_swept=-1.0,life_counted=-1.0,life_previous=-1.0,life_gap=0.0;uint64_t life_walk{};
std::atomic<int> life_rows{30000};std::atomic<bool> life_closed{};
void EntityLifeRow(uintptr_t entity,const EntityLife& l,const char* end) noexcept {
    if(life_rows.load(std::memory_order_relaxed)<=0 || life_rows.fetch_sub(1)<=0)return;
    char node[256]{};if(l.node)sprintf_s(node,",\"node\":[%u,%u,%u,%u,%u,%u]",l.count_birth,l.count_last,l.count_max,l.cap,l.bits_birth,l.bits_last);
    if(l.head){char age[24]{},from[24]{},owner[160]{};   // %.6g: at most 14 characters a float; null when not finite (the step review of c38c9abfd, H1, L4)
        const auto f=[](float v,char (&out)[24]){if(std::isfinite(v))sprintf_s(out,"%.6g",double(v));else strcpy_s(out,"null");};f(l.age_last,age);f(l.from_last,from);
        sprintf_s(owner,",\"owner\":[%d,%d,%s,%s,\"%X\"]",l.state_birth,l.state_last,age,from,l.flags_last);strcat_s(node,owner);}
    char row[720]{};sprintf_s(row,"{\"event\":\"ffx_entity_life\",\"mode\":\"%s\",\"entity\":\"%llX\",\"vt\":\"%llX\",\"kind\":%d,\"masks\":[\"%X\",\"%X\",\"%X\"],\"pos\":[%.3f,%.3f,%.3f],\"first\":%.1f,\"last\":%.1f,\"offers\":%llu,\"open\":%d,\"end\":\"%s\"%s}\n",
        native_ffx_draw?"native":"reference",static_cast<unsigned long long>(entity),static_cast<unsigned long long>(l.vt>image && l.vt<image+0x5000000?l.vt-image:0),l.kind,l.masks[0],l.masks[1],l.masks[2],
        l.position[0],l.position[1],l.position[2],l.first,l.last,static_cast<unsigned long long>(l.offers),std::strcmp(end,"open")==0?1:0,end,node);Log(row);}
void EntityLifeOffered(uintptr_t entity,int32_t kind) noexcept {
    static const bool on=ModuleFlag(L"ffx-entity-life.flag");if(!on || !entity || life_closed.load(std::memory_order_relaxed))return;
    if(SceneMs()<0)return;
    // The identity and the node's state are read outside the lock on every offer; a birth's position (25F9D0) and masks under it.
    uintptr_t vt{},getter{},object{},holder{},resource{};
    if(Read(entity,&vt,8) && vt && Read(vt+0x28,&getter,8) && getter){object=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(getter)(entity);
        if(object && Read(object+0x10,&holder,8) && holder)Read(holder+0x40,&resource,8);}
    const NodeState state=ReadNodeState(entity,vt);
    const std::lock_guard hold(life_lock);
    if(life_closed.load())return;
    const double ms=SceneMs();   // under the lock: first and last never go backwards (the step review's L3)
    if(life_previous>=0){const double gap=ms-life_previous;if(gap>4.0)++life_walk;life_gap=(std::max)(life_gap,gap);}life_previous=ms;
    const auto sweep=[&](double idle){for(auto i=lives.begin();i!=lives.end();){if(life_walk-i->second.last_walk>12 && ms-i->second.last>idle){EntityLifeRow(i->first,i->second,"gone");i=lives.erase(i);}else ++i;}};
    if(ms>90000.0){life_closed=true;sweep(100.0);for(const auto& [e,l]:lives)EntityLifeRow(e,l,"open");
        char row[128]{};sprintf_s(row,"{\"event\":\"ffx_entity_life_end\",\"mode\":\"%s\",\"scene_ms\":%.1f,\"rows_left\":%d}\n",native_ffx_draw?"native":"reference",ms,life_rows.load());Log(row);lives.clear();return;}
    auto found=lives.find(entity);
    // A node freed and another allocated at its address, or a head whose contents moved to a new node (1ADAD80), gets a new
    // particle array (+0x60, allocated by 1ADCA20 when null): its change is a new life too (the step review of 8a722fbad, M1).
    const bool new_array=state.valid && state.particles && found!=lives.end() && found->second.particles && found->second.particles!=state.particles;
    if(found!=lives.end() && (found->second.kind!=kind || found->second.vt!=vt || found->second.object!=object || found->second.resource!=resource || new_array)){
        EntityLifeRow(found->first,found->second,"reborn");lives.erase(found);found=lives.end();}   // the step review's M1
    if(found==lives.end()){found=lives.emplace(entity,EntityLife{}).first;auto& l=found->second;l.first=ms;l.kind=kind;l.vt=vt;l.object=object;l.resource=resource;
        if(object){alignas(16) std::array<float,20> m{};const auto p=reinterpret_cast<uintptr_t(__fastcall*)(void*,uintptr_t)>(image+0x25F9D0)(m.data(),object+0x60);
            if(p){Read(p+0xC,&l.position[0],4);Read(p+0x1C,&l.position[1],4);Read(p+0x2C,&l.position[2],4);}
            if(resource)Read(resource+0x1F8,l.masks,12);}
        for(auto& x:l.position)if(!std::isfinite(x))x=0;
        }
    auto& l=found->second;l.last=ms;l.last_walk=life_walk;++l.offers;
    if(state.valid){if(!l.node){l.node=true;l.count_birth=state.count;l.cap=state.cap;l.bits_birth=state.bits;}   // the first state read, at the birth
        if(state.head){if(!l.head)l.state_birth=state.state;l.head=true;l.state_last=state.state;l.age_last=state.age;l.from_last=state.from;l.flags_last=state.flags;}
        l.count_last=state.count;l.count_max=(std::max)(l.count_max,state.count);l.bits_last=state.bits;if(state.particles)l.particles=state.particles;}
    if(ms-life_swept>=50.0){life_swept=ms;sweep(100.0);}   // the step review's M2: walks missed, not a wall-clock gap alone
    if(ms-life_counted>=1000.0){life_counted=std::floor(ms/1000.0)*1000.0;std::map<std::pair<int32_t,uint32_t>,uint32_t> alive;for(const auto& [e,x]:lives)if(ms-x.last<=100.0)++alive[{x.kind,x.masks[0]}];
        std::string row="{\"event\":\"ffx_entity_life_count\",\"mode\":\"";row+=native_ffx_draw?"native":"reference";char cell[96]{};
        sprintf_s(cell,"\",\"scene_ms\":%.1f,\"walk\":%llu,\"gap_max\":%.1f,\"alive\":[",ms,static_cast<unsigned long long>(life_walk),life_gap);row+=cell;bool first=true;life_gap=0.0;
        for(const auto& [key,n]:alive){sprintf_s(cell,"%s[%d,\"%X\",%u]",first?"":",",key.first,key.second,n);row+=cell;first=false;}row+="]}\n";Log(row.c_str());}}
// I-0141 tranche 14 (the first walk's burst at 14.4 s: natively 122 of 178 smoke entities offered once, their owner at age 0;
// the original offers none at birth at those positions): the newborn ceremony heads, both modes, with ffx-entity-life.flag. A head
// is the type-0x898 effect object's (vtable 3A9C150) embedded first node, object+0xD0; newborn: the object's age (+0xA8) in
// [0, 0.25) s; ceremony: the object's instance (+0x10) created as FXR 732035; on the scene clock in [0, 1500] and [14000, 15500] ms.
// ffx_newborn_draw: the node draw 1ADDE80(node, object, pass) (READ, GhidraMCP 2026-11-20: it offers node+0x78 through the pass's
// vt+0x20 when the count +0x70, the renderer +0x58 and the resource +0x68 are set, the pass's vt+0x30 accepts the renderer's
// vt+0x48 kind and its vt+0x28 accepts the bounds node+0x30): what it reads and whether it offered (EntityOffered on this thread).
// ffx_newborn_update: the same heads in the update 1ADD510 (the count before and after, the bounds after, the thread).
// At most 6000 rows of each.
thread_local int newborn_offers{};
thread_local int newborn_node_draws{};
// ffx_draw_gate (tranche 14): each scene-control draw 24E790 on pass 0x45 in the windows [0, 80] and [14370, 14440] ms of the scene
// clock, both modes, with ffx-entity-life.flag: 24E790's gates (the GX display [144961AB0]'s +0x3E1, -1 when null; the scene
// control's +0x5BE, +0x20 and +0x590), the library's draw list at the call (1AD1630 / 249680 / 249690 / 1AC9BD0, as
// reference_ffx_draw_list: objects of the type 0x898 (vt 3A9C150), those of a FXR 732035 instance with an age in [0, 0.25) s, those
// at age 0), and the nodes 1ADDE80 drew and the entities offered during the call (this thread). At most 400 rows.
std::atomic<int> draw_gate_rows{400};
// ffx_frame_ring (tranche 14): the moments (QPC ms) of the last 12 pass-0x45 draws and of the last 12 node-update bursts (1ADD510
// calls more than 3 ms after the previous one), written beside each window's first ffx_draw_gate row on the scene clock: whether the
// walk before the newborns' birth came just before it, and how many updates ran between two walks.
std::mutex moment_lock;std::deque<double> draw_moments,update_moments;double update_previous{};   // update_previous: under moment_lock
void NoteMoment(std::deque<double>& ring,double gap) noexcept {
    const double now=QpcMs();const std::lock_guard hold(moment_lock);
    if(!ring.empty() && now-ring.back()<gap)return;ring.push_back(now);if(ring.size()>12)ring.pop_front();}
// An update call: a moment when it comes more than 3 ms after the previous call (any thread), its time read under the lock so the
// calls are taken in the lock's order; the time is the call's return (NewbornUpdateRow runs after 1ADD510), the draws' their start.
void NoteUpdateCall() noexcept {
    const std::lock_guard hold(moment_lock);const double now=QpcMs(),before=update_previous;update_previous=now;
    if(now-before<=3.0)return;update_moments.push_back(now);if(update_moments.size()>12)update_moments.pop_front();}
void FrameRingRow(double at_ms) noexcept {
    const int64_t from=scene_clock_from.load(std::memory_order_relaxed);if(from<0)return;
    LARGE_INTEGER rate{};QueryPerformanceFrequency(&rate);const double origin=double(from)*1000.0/double(rate.QuadPart);
    std::string row="{\"event\":\"ffx_frame_ring\",\"mode\":\"";row+=native_ffx_draw?"native":"reference";char cell[48]{};
    sprintf_s(cell,"\",\"scene_ms\":%.2f,\"draws\":[",at_ms);row+=cell;
    const std::lock_guard hold(moment_lock);
    for(size_t i=0;i<draw_moments.size();++i){sprintf_s(cell,"%s%.2f",i?",":"",draw_moments[i]-origin);row+=cell;}
    row+="],\"updates\":[";
    for(size_t i=0;i<update_moments.size();++i){sprintf_s(cell,"%s%.2f",i?",":"",update_moments[i]-origin);row+=cell;}
    Log((row+"]}\n").c_str());}
DrawGate DrawGateEnter(uintptr_t sc,uintptr_t pass) noexcept {
    DrawGate gate{};static const bool on=ModuleFlag(L"ffx-entity-life.flag");if(!on || !sc || !pass)return gate;
    int32_t id{};Read(pass+0x124,&id,4);if(id!=0x45)return gate;
    const double ms=SceneMs();
    static std::atomic<int> ring_rows{2};static std::atomic<bool> late_ring{};   // before this draw's own moment: the ring as it was
    if(ms>=0.0 && ms<=80.0 && ring_rows.load()==2 && ring_rows.fetch_sub(1)==2)FrameRingRow(ms);
    if(ms>=14370.0 && ms<=14440.0 && !late_ring.exchange(true))FrameRingRow(ms);
    NoteMoment(draw_moments,1.0);
    if(ms<0.0 || (ms>80.0 && ms<14370.0) || ms>14440.0 || draw_gate_rows.load(std::memory_order_relaxed)<=0)return gate;
    gate.ms=ms;gate.offers=newborn_offers;gate.nodes=newborn_node_draws;return gate;}
void DrawGateLeave(const DrawGate& gate,uintptr_t sc,uintptr_t pass) noexcept {
    if(gate.ms<-100.0 || draw_gate_rows.fetch_sub(1)<=0)return;
    uintptr_t display{},mgr{},mgr8{};int display_3e1=-1;uint8_t s5be{},s20{},s590{};int32_t id{};
    if(Read(image+0x4961AB0,&display,8) && display){uint8_t b{};if(Read(display+0x3E1,&b,1))display_3e1=b;}
    Read(sc+0x5BE,&s5be,1);Read(sc+0x20,&s20,1);Read(sc+0x590,&s590,1);Read(pass+0x124,&id,4);
    uint32_t listed{},newborn{},age0{};Read(sc+0x18,&mgr,8);if(mgr)Read(mgr+8,&mgr8,8);
    if(mgr8)for(auto g=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x1AD1630)(mgr8);g && listed<100000;g=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x249680)(g))
        for(auto o=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x249690)(g);o && listed<100000;o=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x1AC9BD0)(o)){
            uintptr_t vt{},instance{};float age{};if(!Read(o,&vt,8) || vt!=image+0x3A9C150)continue;++listed;
            if(!Read(o+0xA8,&age,4) || !(age>=0.f && age<0.25f) || !Read(o+0x10,&instance,8))continue;
            uint32_t fxr{};{const std::lock_guard hold(instance_fxr_lock);const auto f=instance_fxr.find(instance);if(f!=instance_fxr.end())fxr=f->second;}
            if(fxr!=732035)continue;++newborn;if(age==0.f)++age0;}
    char row[512]{};sprintf_s(row,"{\"event\":\"ffx_draw_gate\",\"mode\":\"%s\",\"scene_ms\":%.2f,\"thread\":%lu,\"pass\":\"%X\",\"display_3e1\":%d,\"sc_5be\":%u,\"sc_20\":%u,\"sc_590\":%u,\"listed\":%u,\"listed_newborn\":%u,\"listed_age0\":%u,\"node_draws\":%d,\"offers\":%d}\n",native_ffx_draw?"native":"reference",gate.ms,GetCurrentThreadId(),id,display_3e1,s5be,s20,s590,listed,newborn,age0,
        newborn_node_draws-gate.nodes,newborn_offers-gate.offers);Log(row);}
using NodeDraw=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);NodeDraw node_draw_original{};
std::atomic<int> newborn_draw_rows{6000},newborn_update_rows{6000};
struct NewbornHead {double ms{};float age{};uint32_t flags{},state{},count{};uintptr_t renderer{},resource{};float bounds[8]{};};
bool ReadNewbornHead(uintptr_t node,NewbornHead& h) noexcept {
    static const bool on=ModuleFlag(L"ffx-entity-life.flag");if(!on || node<0xD0)return false;
    h.ms=SceneMs();if(h.ms<0 || (h.ms>1500.0 && h.ms<14000.0) || h.ms>15500.0)return false;
    const uintptr_t object=node-0xD0;uintptr_t vt{},instance{};
    if(!Read(object,&vt,8) || vt!=image+0x3A9C150 || !Read(object+0xA8,&h.age,4) || !(h.age>=0.f && h.age<0.25f) || !Read(object+0x10,&instance,8) || !instance)return false;
    uint32_t fxr{};{const std::lock_guard hold(instance_fxr_lock);const auto f=instance_fxr.find(instance);if(f!=instance_fxr.end())fxr=f->second;}
    if(fxr!=732035)return false;
    Read(object+0xB0,&h.flags,4);Read(object+0xBC,&h.state,4);Read(node+0x70,&h.count,4);Read(node+0x58,&h.renderer,8);Read(node+0x68,&h.resource,8);Read(node+0x30,h.bounds,32);
    for(auto& b:h.bounds)if(!std::isfinite(b))b=-1e30f;
    return true;}
std::string NewbornBounds(const NewbornHead& h){std::string out="[";char cell[48]{};for(int i=0;i<8;++i){sprintf_s(cell,"%s%.6g",i?",":"",h.bounds[i]);out+=cell;}return out+"]";}   // %.6g: a reset box's 3.4e38 overflowed %.2f's 24-byte cell (0xC0000409, i141-t14-n2)
void __fastcall NodeDrawObserved(uintptr_t node,uintptr_t object,uintptr_t pass) noexcept {
    NewbornHead h{};const bool due=ReadNewbornHead(node,h) && node==object+0xD0 && newborn_draw_rows.load(std::memory_order_relaxed)>0;
    ++newborn_node_draws;const int offers=newborn_offers;node_draw_original(node,object,pass);
    if(!due || newborn_draw_rows.fetch_sub(1)<=0)return;
    char row[640]{};sprintf_s(row,"{\"event\":\"ffx_newborn_draw\",\"mode\":\"%s\",\"scene_ms\":%.2f,\"node\":\"%llX\",\"age\":%.4f,\"state\":%u,\"flags\":\"%X\",\"count\":%u,\"renderer\":%d,\"resource\":%d,\"offered\":%d,\"thread\":%lu,\"bounds\":%s}\n",
        native_ffx_draw?"native":"reference",h.ms,static_cast<unsigned long long>(node),h.age,h.state,h.flags,h.count,h.renderer?1:0,h.resource?1:0,newborn_offers-offers,GetCurrentThreadId(),NewbornBounds(h).c_str());Log(row);}
void NewbornUpdateRow(uintptr_t node,float dt,uint32_t before) noexcept {
    {static const bool on=ModuleFlag(L"ffx-entity-life.flag");if(!on)return;}
    NoteUpdateCall();   // ffx_frame_ring's update bursts
    if(newborn_update_rows.load(std::memory_order_relaxed)<=0)return;
    NewbornHead h{};if(!ReadNewbornHead(node,h) || newborn_update_rows.fetch_sub(1)<=0)return;
    char step[24]{};if(std::isfinite(dt))sprintf_s(step,"%.6g",dt);else strcpy_s(step,"null");   // the age is finite (ReadNewbornHead)
    char row[640]{};sprintf_s(row,"{\"event\":\"ffx_newborn_update\",\"mode\":\"%s\",\"scene_ms\":%.2f,\"node\":\"%llX\",\"age\":%.4f,\"state\":%u,\"flags\":\"%X\",\"dt\":%s,\"before\":%u,\"count\":%u,\"thread\":%lu,\"bounds\":%s}\n",
        native_ffx_draw?"native":"reference",h.ms,static_cast<unsigned long long>(node),h.age,h.state,h.flags,step,before,h.count,GetCurrentThreadId(),NewbornBounds(h).c_str());Log(row);}
uint8_t __fastcall EntityOffered(uintptr_t pass,uintptr_t entity,int32_t kind,uintptr_t box,float bias) noexcept {
    {static const bool life=ModuleFlag(L"ffx-entity-life.flag");if(life)++newborn_offers;}   // I-0141 tranche 14: ffx_newborn_draw's « offered »
    uintptr_t ctx{};if(pass)std::memcpy(&ctx,reinterpret_cast<const void*>(pass+8),8);const bool native_context=ctx && ctx==Context();if(native_context)++entity_calls;
    int32_t pass_id{};if(ctx)Read(ctx+0x124,&pass_id,4);
    if(pass_id==0x45)EntityLifeOffered(entity,kind);
    const bool logged=pass_id==0x45 && entity_rows.load()>0 && entity_rows.fetch_sub(1)>0;
    if(!logged)return entity_original(pass,entity,kind,box,bias);
    {static std::atomic<int> context_rows{1};   // I-0141 tranche 11: the pass context once a session, its +0x100..+0x600 (the sub-pass bytes +0x426.. among them) and the pass object
        if(context_rows.fetch_sub(1)>0){char head[160]{};sprintf_s(head,"{\"event\":\"ffx_pass_context\",\"mode\":\"%s\",\"ctx\":\"%llX\",\"pass\":\"",native_ffx_draw?"native":"reference",static_cast<unsigned long long>(ctx));
            Log((head+ffx_draw_observe::HexWords(pass,0x40)+"\",\"words_100\":\""+ffx_draw_observe::HexWords(ctx+0x100,0x500)+"\"}\n").c_str());}}
    uint32_t masks[3]{},scene_mask{},pass_masks[2]{};uintptr_t sc{},camera{},list{},object_seen{},entity_vt{};float position[3]{};uint32_t culled=2;
    void* back[2]{};const USHORT backs=RtlCaptureStackBackTrace(1,2,back,nullptr);   // I-0141 tranche 11: who offers it (the twice-offered smoke)
    const auto at_rva=[&](int i){const auto f=reinterpret_cast<uintptr_t>(back[i]);return static_cast<unsigned long long>(i<backs && f>image && f<image+0x5000000?f-image:0);};
    {uintptr_t vt{},getter{};if(entity && Read(entity,&vt,8) && vt && Read(vt+0x28,&getter,8) && getter){
            const auto object=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(getter)(entity);uintptr_t holder{},resource{};object_seen=object;entity_vt=vt;
            if(object){alignas(16) std::array<float,20> m{};const auto at=reinterpret_cast<uintptr_t(__fastcall*)(void*,uintptr_t)>(image+0x25F9D0)(m.data(),object+0x60);
                if(at){Read(at+0xC,&position[0],4);Read(at+0x1C,&position[1],4);Read(at+0x2C,&position[2],4);}
                if(Read(object+0x10,&holder,8) && holder && Read(holder+0x40,&resource,8) && resource)Read(resource+0x1F8,masks,12);}}
        if(Read(pass+0x10,&sc,8) && sc)Read(sc+0x14C,&scene_mask,4);Read(ctx+0x4B8,pass_masks,8);
        if(Read(ctx+0x418,&camera,8) && camera)culled=reinterpret_cast<uint8_t(__fastcall*)(uintptr_t,uintptr_t)>(image+0x29AA10)(camera,entity)?1:0;}
    uint32_t before{};if(native_context)before=uint32_t(commands.size());else if(Read(ctx+0x4B0,&list,8) && list)Read(list+0x7C,&before,4);
    const uint8_t answer=entity_original(pass,entity,kind,box,bias);
    uint32_t after{};if(native_context)after=uint32_t(commands.size());else if(list)Read(list+0x7C,&after,4);
    std::array<float,8> bounds{};if(box)Read(box,bounds.data(),32);for(auto& x:bounds)if(!std::isfinite(x))x=0;   // the argument 2997E0 takes as its sort position (+0..+0xF), and the next words
    char row[768]{};sprintf_s(row,"{\"event\":\"ffx_entity_offered\",\"pos_arg\":[%g,%g,%g,%g,%g,%g,%g,%g],\"mode\":\"%s\",\"kind\":%d,\"pos\":[%.3f,%.3f,%.3f],\"masks\":[\"%X\",\"%X\",\"%X\"],\"scene_mask\":\"%X\",\"pass_masks\":[\"%X\",\"%X\"],\"culled\":%u,\"recorded\":%d,\"entity\":\"%llX\",\"vt\":\"%llX\",\"object\":\"%llX\",\"ctx\":\"%llX\",\"ours\":%d,\"callers\":[\"%llX\",\"%llX\"]}\n",
        bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5],bounds[6],bounds[7],native_ffx_draw?"native":"reference",kind,std::isfinite(position[0])?position[0]:0.f,std::isfinite(position[1])?position[1]:0.f,std::isfinite(position[2])?position[2]:0.f,
        masks[0],masks[1],masks[2],scene_mask,pass_masks[0],pass_masks[1],culled,int(after)-int(before),static_cast<unsigned long long>(entity),
        static_cast<unsigned long long>(entity_vt>image && entity_vt<image+0x5000000?entity_vt-image:0),static_cast<unsigned long long>(object_seen),static_cast<unsigned long long>(ctx),native_context?1:0,at_rva(0),at_rva(1));Log(row);return answer;}
// E07 (on by default): the GPU-particle simulation 2A6A30 binds its update textures S+0x2A10 (CS t0 csInputTex) and S+0x37D0 (t4
// csTargetTex) by reading each GXTexture2D's +0x48 directly (no vt+0x10 call), null natively (Native1999: S+0x2A10 a GXTexture2D,
// vtable 3D1D528, +0x48 zero). For the call only, a GXTexture2D the native texture registry owns carries its FFX texture token at
// +0x48 (the token the draw side resolves to the native view), zero again on return. Witness (ffx-record.flag): the objects, every
// 1200th call. 11 arguments, passed through.
using Simulation=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t,uintptr_t);Simulation simulation_original{};
std::atomic<uint64_t> simulation_textures_lent{},simulation_textures_unowned{};
// E07 witness, both modes (Reference with ffx-draw-observe.flag, Native with ffx-record.flag): per particle system S
// the calls of 2A6A30 that simulate (S+0x3B15 clear on entry), the emissions (S+0x3968 the pending count, emitted when > 0), their
// sum and largest, the capacity S+0x2890, active S+0x3B39, the time arguments (param 7, 8); every 1200th simulating call, the
// systems seen with the CPU images the CS constants come from: ccbSceneParam S+0x40, ccbEmitionSeed S+0x1230 (written by the call,
// read after it), its m_TexNumInfo..m_WanderingVisionResetTime S+0xA0, ccbEmitionSeed S+0x1230 and its m_EmittorLongevity..
// S+0x1430 (written by the call, read after it), the force-field sources S+0x490 and S+0xDA0, 16 words each.
struct SimulationTally {uint64_t calls{},emits{},emitted{};uint32_t largest{},capacity{};uint8_t active{};float t7{},t8{};};
std::mutex simulation_tally_lock;std::map<uintptr_t,SimulationTally> simulation_tally;uint64_t simulation_tally_calls{};
void SimulationWitness(const char* mode,uintptr_t s,uintptr_t a7,uintptr_t a8,bool after) noexcept {
    if(!s)return;static thread_local uint32_t pending{};static thread_local bool simulating{};
    if(!after){uint8_t done{};Read(s+0x3B15,&done,1);simulating=!done;pending=0;if(simulating)Read(s+0x3968,&pending,4);return;}
    if(!simulating)return;
    const std::lock_guard hold(simulation_tally_lock);auto& t=simulation_tally[s];++t.calls;
    if(static_cast<int32_t>(pending)>0){++t.emits;t.emitted+=pending;t.largest=std::max(t.largest,pending);}
    Read(s+0x2890,&t.capacity,4);Read(s+0x3B39,&t.active,1);uint32_t f7=uint32_t(a7),f8=uint32_t(a8);std::memcpy(&t.t7,&f7,4);std::memcpy(&t.t8,&f8,4);
    if(++simulation_tally_calls%1200!=600)return;
    std::string row=std::string("{\"event\":\"ffx_simulation_witness\",\"mode\":\"")+mode+"\",\"calls\":"+std::to_string(simulation_tally_calls)+",\"systems\":[";
    size_t n=0;for(const auto& [system,v]:simulation_tally){if(n++>=8)break;char cell[400]{};
        sprintf_s(cell,"%s{\"s\":\"%llX\",\"calls\":%llu,\"emits\":%llu,\"emitted\":%llu,\"largest\":%u,\"capacity\":%u,\"active\":%u,\"t\":[%g,%g]",n>1?",":"",
            static_cast<unsigned long long>(system),static_cast<unsigned long long>(v.calls),static_cast<unsigned long long>(v.emits),static_cast<unsigned long long>(v.emitted),v.largest,v.capacity,v.active,v.t7,v.t8);
        row+=cell;
        for(const auto& [name,at]:std::initializer_list<std::pair<const char*,uint32_t>>{{"scene",0x40},{"emit_info",0xA0},{"seed",0x1230},{"longevity",0x1430},{"force_a",0x490},{"force_b",0xDA0}}){
            std::array<float,16> w{};Read(system+at,w.data(),sizeof(w));row+=std::string(",\"")+name+"\":[";
            for(size_t i=0;i<w.size();++i){char f[24]{};sprintf_s(f,"%s%.5g",i?",":"",w[i]);row+=f;}row+="]";}
        row+="}";}
    row+="]}\n";Log(row.c_str());}
// Snow witness (both modes: Reference with ffx-draw-observe.flag, Native with ffx-record.flag): slot 27 (264230) fills the draw
// context's VS block fog words (+0x2D0..+0x2E7; the screen size is 263330's) from the scene parameter table it reaches through [+0xA60] vt+0x30 -> +0x18
// (count +0x254 > 0x1B and +0x25C > 6; the table +0x260, the index +0x248). After each call: whether that table qualifies, its words
// +0x50, +0x58, +0x64, +0x68 at the index, the context's +0x2D0..+0x2EC and +0x6A0..+0x6AC; every 3000th call one row.
using SlotDraw=void(__fastcall*)(uintptr_t);SlotDraw slot27_original{};
std::mutex slot27_lock;uint64_t slot27_calls{},slot27_tables{},slot27_qualified{};
// I-0113 tranche 23 (gap 22: the distortion quads of pass 0x2D, VS 2A1C / PS 2AC7, unblended, each writing the frame copy at
// its own offset): with ffx-series.flag, in both modes alike, 3 s of every distortion draw (t0 the frame copy) from the moment
// ffx-series-go.txt appears beside the module (looked for every 250 ms): the time since then (ms), the call's order, the FXR blend
// type +0x960, the PS image's g_color, the wave words (+0x40..+0x6F: near/far, g_waveTime, the clamp, g_waveStrength, the two
// scroll distances), g_vColorScale, softSpriteParam, and the VS image (+0x140, VSCommonBuffer): g_worldViewProjectionMatrix
// (+0xC0), g_eye (+0x150) and g_fbTexCoef (+0x160): which quads overlap, in what order, against the original's.
void DistortionSeriesRow(uintptr_t c) noexcept {
    static const bool series=ModuleFlag(L"ffx-series.flag");if(!series || !c)return;
    auto& from=ffx_draw_observe::series_from;static std::atomic<uint64_t> checked{},order{};static std::atomic<int> rows{};
    // I-0113 tranche 31 (gap 16), with ffx-series-auto.flag as well (the step review's M1: the go file's window for gap 22's
    // barrier runs unchanged without it): a distortion quad blended otherwise than the default (+0x960 not 0: the 608 burst's
    // radial blurs) opens the window, without ffx-series-go.txt, and again 5 s after the last one opened (one window a cast);
    // each opening writes a row with the rows left (the budgets are per run).
    static const bool automatic=ModuleFlag(L"ffx-series-auto.flag");
    uint32_t blend{};uintptr_t t0{},copy{};const bool quad=Read(c+0x968,&t0,8) && Read(c+0xA70,&copy,8) && t0 && t0==copy && Read(c+0x960,&blend,4);
    if(automatic && quad && blend && ffx_draw_observe::SeriesMs()<0){LARGE_INTEGER now{},rate{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&rate);
        auto before=from.load(std::memory_order_relaxed);
        if((before<0 || double(now.QuadPart-before)*1000.0/double(rate.QuadPart)>5000) && from.compare_exchange_strong(before,now.QuadPart)){char row[160]{};
            sprintf_s(row,"{\"event\":\"ffx_distortion_series_open\",\"mode\":\"%s\",\"trigger\":\"blend\",\"blend\":%u,\"rows_used\":%d}\n",native_ffx_draw?"native":"reference",blend,rows.load());Log(row);}}
    if(!automatic && from.load(std::memory_order_relaxed)<0){const uint64_t tick=GetTickCount64();if(tick<checked.load(std::memory_order_relaxed)+250)return;checked=tick;
        const auto go=ModuleSibling(L"ffx-series-go.txt");if(go.empty() || GetFileAttributesW(go.c_str())==INVALID_FILE_ATTRIBUTES)return;
        LARGE_INTEGER now{};QueryPerformanceCounter(&now);int64_t expected=-1;from.compare_exchange_strong(expected,now.QuadPart);}
    const double ms=ffx_draw_observe::SeriesMs();   // -1 past the 3 s
    if(ms<0 || !quad || rows.fetch_add(1)>=4000)return;
    std::array<float,4> color{},scale{},soft{};std::array<float,12> wave{};std::array<float,16> wvp{};std::array<float,5> eye{};
    // Tranche 31: the radial blur's words (g_screenCenter, g_radialDistance at +0x70; g_ps_ColorBlendType at +0x17C;
    // g_ps_RadialBlurSamplingLengthMaxX/Y at +0x1E0), the 608 programme reading them (PS 2AE9: 16 taps toward the centre).
    std::array<float,4> radial{};std::array<float,2> radial_max{};int32_t colour_blend{};
    Read(c+0x4C0+0x70,radial.data(),16);Read(c+0x4C0+0x17C,&colour_blend,4);Read(c+0x4C0+0x1E0,radial_max.data(),8);
    Read(c+0x4C0,color.data(),16);Read(c+0x4C0+0x40,wave.data(),48);Read(c+0x4C0+0x90,scale.data(),16);Read(c+0x4C0+0x1D0,soft.data(),16);
    Read(c+0x140+0xC0,wvp.data(),64);Read(c+0x140+0x150,eye.data(),20);   // g_eye (12 bytes) then g_fbTexCoef at +0x160
    std::string row;char cell[256]{};
    sprintf_s(cell,"{\"event\":\"ffx_distortion_series\",\"mode\":\"%s\",\"ms\":%.3f,\"o\":%llu,\"thread\":%lu,\"blend\":%u,\"color\":[%.5g,%.5g,%.5g,%.5g],\"wave\":[",native_ffx_draw?"native":"reference",ms,
        static_cast<unsigned long long>(order.fetch_add(1)),GetCurrentThreadId(),blend,color[0],color[1],color[2],color[3]);row=cell;
    const auto list=[&](const float* v,size_t n){for(size_t i=0;i<n;++i){if(std::isfinite(v[i]))sprintf_s(cell,"%s%.6g",i?",":"",v[i]);else sprintf_s(cell,"%snull",i?",":"");row+=cell;}};
    list(wave.data(),wave.size());row+="],\"scale\":[";list(scale.data(),4);row+="],\"soft\":[";list(soft.data(),4);
    row+="],\"wvp\":[";list(wvp.data(),16);row+="],\"eye\":[";list(eye.data(),3);row+="],\"coef\":[";list(eye.data()+3,2);
    row+="],\"radial\":[";list(radial.data(),4);row+="],\"radial_max\":[";list(radial_max.data(),2);row+="],\"colour_blend\":"+std::to_string(colour_blend)+"}\n";Log(row.c_str());}
// I-0113 tranche 32 (gap 16, the native's extra red negatives): PS 2AFA (the soft billboard, `t30/ffx-ps-2AFA-9FCB018A.dxbc`) takes
// its colour as v0.rgb + softSpriteParam.w x (g_vAmbientSH[0..2].w - v0.rgb), normalised, so an SH .w word lower in one channel
// pulls that channel negative. In the ffx-series window (both modes alike), every draw whose PS image (+0x4C0) has a non-zero
// softSpriteParam.w: g_vAmbientSH (+0xA0, three rows), softSpriteParam (+0x1D0), g_vColorScale (+0x90), g_ps_ColorBlendType
// (+0x17C), g_ps_FogMultMode (+0x1BC), g_ps_bPreAlphaMult (+0x220), the FXR blend type +0x960, the row's order (rows only, racing threads may swap two) and thread and the
// VS image's world-view-projection (to pair the modes' quads; the PS's identity is not in the image); at most 2000 rows a run.
void SoftSpriteSeriesRow(uintptr_t c) noexcept {
    static const bool series=ModuleFlag(L"ffx-series.flag");if(!series || !c || ffx_draw_observe::SeriesMs()<0)return;
    std::array<float,4> soft{};if(!Read(c+0x4C0+0x1D0,soft.data(),16) || soft[3]==0.f)return;
    static std::atomic<int> rows{};static std::atomic<uint64_t> order{};
    if(rows.load(std::memory_order_relaxed)>=2000 || rows.fetch_add(1)>=2000)return;   // the reviews: the counter stops, no wrap
    std::array<float,12> sh{};std::array<float,4> scale{};std::array<float,16> wvp{};int32_t colour_blend{},fog{},pre{};uint32_t blend{};
    Read(c+0x4C0+0xA0,sh.data(),48);Read(c+0x4C0+0x90,scale.data(),16);Read(c+0x4C0+0x17C,&colour_blend,4);Read(c+0x4C0+0x1BC,&fog,4);Read(c+0x4C0+0x220,&pre,4);Read(c+0x960,&blend,4);
    Read(c+0x140+0xC0,wvp.data(),64);
    // Tranche 33: the SH rows' .w words were 0 natively, 1 or the probe's L0 in the original, their xyz equal (i113-t33-orbsh-near-*);
    // 264230 scales row k's .w word last by [[c+0x10]+8]+0x110+4k (its xyz by constants; 140265BF3): that chain, its two objects' vtables.
    // Tranche 33's step review (M1, L1): the four floats, and whether the owner is the native effect pass context.
    uintptr_t holder{},owner{},holder_vt{},owner_vt{};std::array<float,4> w_scales{-1.f,-1.f,-1.f,-1.f};
    if(Read(c+0x10,&holder,8) && holder){Read(holder,&holder_vt,8);if(Read(holder+8,&owner,8) && owner){Read(owner,&owner_vt,8);Read(owner+0x110,w_scales.data(),16);}}
    for(auto& w:w_scales)if(!std::isfinite(w))w=-2.f;   // valid JSON
    const float w_scale=w_scales[0];const int owner_is_pass=native_ffx_draw && owner==native_ffx::Context()?1:0;
    const auto rel=[](uintptr_t v){return static_cast<unsigned long long>(v>=image && v<image+0x10000000?v-image:0);};
    std::string row;char cell[400]{};
    sprintf_s(cell,"{\"event\":\"ffx_soft_sprite_series\",\"mode\":\"%s\",\"ms\":%.3f,\"o\":%llu,\"thread\":%lu,\"blend\":%u,\"colour_blend\":%d,\"fog\":%d,\"pre_alpha\":%d,\"w_scale\":%.6g,\"w_scales\":[%.6g,%.6g,%.6g,%.6g],\"owner_is_pass\":%d,\"holder_vt\":%llu,\"owner_vt\":%llu,\"sh\":[",
        native_ffx_draw?"native":"reference",ffx_draw_observe::SeriesMs(),static_cast<unsigned long long>(order.fetch_add(1)),GetCurrentThreadId(),blend,colour_blend,fog,pre,
        w_scale,w_scales[0],w_scales[1],w_scales[2],w_scales[3],owner_is_pass,rel(holder_vt),rel(owner_vt));row=cell;
    const auto list=[&](const float* v,size_t n){for(size_t i=0;i<n;++i){if(std::isfinite(v[i]))sprintf_s(cell,"%s%.6g",i?",":"",v[i]);else sprintf_s(cell,"%snull",i?",":"");row+=cell;}};
    list(sh.data(),sh.size());row+="],\"soft\":[";list(soft.data(),4);row+="],\"scale\":[";list(scale.data(),4);row+="],\"wvp\":[";list(wvp.data(),16);row+="]}\n";Log(row.c_str());}
std::atomic<int> vs_image_rows_left{};   // I-0141: the next VSCommonBuffer images logged whole (F9, with the pixel-shader blocks)
void __fastcall Slot27Observed(uintptr_t c) noexcept {
    slot27_original(c);
    DistortionSeriesRow(c);
    SoftSpriteSeriesRow(c);
    {   // I-0113 gap 12 (Flame Fan's fan, both modes): F9 (game window in front) logs the next 3000 draws' PS image and blocks (I-0141: 300 before; the tick in ms; the VS and patch rows stay 300 and 120) (+0x4C0, PSCommonBuffer):
        // I-0141: F13 (Run-AlphaCast -SweepKeyAt <s>,124) is the light press: the same rows without the 3000 draws and the VS and patch rows,
        // which slow the native to about 28 ms a frame. The blend type +0x960, g_ps_nearFar/Distance/far (+64), g_vColorScale (+144), g_ps_TexBlendType/2/3 and ColorBlendType (+368),
        // FogMultMode (+444), softSpriteParam (+464), bPreAlphaMult (+544), the textures +0x968/+0x970 and whether t0 is the frame copy. 4 presses.
        static std::atomic<int> left{},presses{};static std::atomic<bool> down_before{};static std::atomic<uint64_t> told{};
        static std::atomic<uint64_t> polled{};const uint64_t now=GetTickCount64();   // the key read at most every 10 ms (step review L3)
        // I-0141 tranche 12: ffx-entity-arm.txt's moment on the scene clock is a light press too (ScenePressDue, once).
        if(now-polled.load(std::memory_order_relaxed)>=10){polled=now;DWORD owner{};GetWindowThreadProcessId(GetForegroundWindow(),&owner);const bool ours=owner==GetCurrentProcessId(),light=ScenePressDue() || (ours && (GetAsyncKeyState(VK_F13)&0x8000)),down=light || (ours && (GetAsyncKeyState(VK_F9)&0x8000));
            if(down && !down_before.exchange(true)){if(presses.fetch_add(1)<4){
                // I-0141 (the native draws effects the original does not): the inputs of the entity tests in 2997E0 (FFX-DRAW
                // « N5 reading »): the options (quality) object [GXRS+0x5360] (+0x00..+0x7F: its EffectsQuality FFX environment block +0x48..+0x64, the water gate +0x74) and the settings object [GXRS+0x5368]
                // (+0x100..+0x1BF, its +0x158/+0x170/+0x188 read there), as raw words, one row a press (ffx_visibility_inputs).
                uintptr_t root{},options{},quality{};std::array<uint32_t,0x20> o{};std::array<uint32_t,0x30> q{};
                if(Read(image+0x4961B50,&root,8) && root){Read(root+0x5360,&options,8);Read(root+0x5368,&quality,8);}
                const bool ok_o=options && Read(options,o.data(),sizeof(o)),ok_q=quality && Read(quality+0x100,q.data(),sizeof(q));
                std::string vis="{\"event\":\"ffx_visibility_inputs\",\"mode\":\"";vis+=native_ffx_draw?"native":"reference";char w[16]{};
                vis+="\",\"read\":[";vis+=ok_o?"1,":"0,";vis+=ok_q?"1":"0";vis+="],\"options\":\"";for(const auto x:o){sprintf_s(w,"%08X",x);vis+=w;}
                vis+="\",\"quality_100\":\"";for(const auto x:q){sprintf_s(w,"%08X",x);vis+=w;}vis+="\"}\n";Log(vis.c_str());
                TextureCensusRow(native_ffx_draw?"native":"reference");SpawnTallyRows();ffx_draw_observe::camera_rows=6;ffx_draw_observe::frustum_rows=3;entity_rows=1500;if(!light){left=3000;if(native_ffx_draw){effect_draw::RequestPatchRows(120);effect_draw::RequestVsRows(300);}else{ffx_draw_observe::patch_watch=true;ffx_draw_observe::patch_left=120;vs_image_rows_left=300;}}}}else if(!down)down_before=false;}
        if(c && left.load()>0 && left.fetch_sub(1)>0){uint32_t blend{};std::array<float,3> near_far{};std::array<float,4> scale{},soft{};std::array<int32_t,4> blends{};int32_t fog{},pre{};uintptr_t t0{},t1{},copy{};const uint64_t n_row=told.fetch_add(1);
            Read(c+0x960,&blend,4);Read(c+0x4C0+64,near_far.data(),12);Read(c+0x4C0+144,scale.data(),16);Read(c+0x4C0+368,blends.data(),16);Read(c+0x4C0+444,&fog,4);
            Read(c+0x4C0+464,soft.data(),16);Read(c+0x4C0+544,&pre,4);Read(c+0x968,&t0,8);Read(c+0x970,&t1,8);Read(c+0xA70,&copy,8);
            // The step review of f3afadfb3: the tick from the performance counter (GetTickCount64 steps by 10 to 16 ms), and a non-finite
            // word written as 1e38 and counted in "nonfinite", so that the row stays JSON.
            int nonfinite{};for(float* v:{near_far.data(),near_far.data()+1,near_far.data()+2,scale.data(),scale.data()+1,scale.data()+2,scale.data()+3,soft.data(),soft.data()+1,soft.data()+2,soft.data()+3})
                if(!std::isfinite(*v)){*v=1e38f;++nonfinite;}
            LARGE_INTEGER qpc{},rate{};QueryPerformanceCounter(&qpc);QueryPerformanceFrequency(&rate);const double ms=1000.0*double(qpc.QuadPart)/double(rate.QuadPart);
            char row[600]{};sprintf_s(row,"{\"event\":\"ffx_ps_image\",\"mode\":\"%s\",\"n\":%llu,\"ms\":%.3f,\"nonfinite\":%d,\"blend\":%u,\"near_far\":[%g,%g,%g],\"colour_scale\":[%g,%g,%g,%g],\"tex_blend\":[%d,%d,%d],\"color_type\":%d,"
                "\"fog\":%d,\"soft\":[%g,%g,%g,%g],\"pre_alpha\":%d,\"t0_copy\":%d,\"two_textures\":%d}\n",native_ffx_draw?"native":"reference",static_cast<unsigned long long>(n_row),ms,nonfinite,blend,
                near_far[0],near_far[1],near_far[2],scale[0],scale[1],scale[2],scale[3],blends[0],blends[1],blends[2],blends[3],fog,soft[0],soft[1],soft[2],soft[3],pre,int(t0 && t0==copy),int(t1!=0));Log(row);
            // I-0141 (the ceremony's smoke): the same draw's blocks whole, as 264230 uploads them: PSCommonBuffer (+0x4C0, 0x230: g_color,
            // the light colours, the hemisphere, the ambient SH, the colour scale...), PSLightBuffer (+0x6F0, 0x190; 264230 uploads it for
            // the lit programmes only: elsewhere it holds an earlier draw's) and cbFog (+0x880, 0xE0), rows ffx_ps_block with the same n
            // as the ffx_ps_image row above. Each 32-bit word as 8 hex digits (the blocks mix floats and integers: the step review's H1);
            // "read" says which of the three reads succeeded (L1).
            std::array<uint32_t,0x230/4> ps{};std::array<uint32_t,0x190/4> light{};std::array<uint32_t,0xE0/4> fog_block{};
            const bool ok_ps=Read(c+0x4C0,ps.data(),sizeof(ps)),ok_light=Read(c+0x6F0,light.data(),sizeof(light)),ok_fog=Read(c+0x880,fog_block.data(),sizeof(fog_block));
            std::string full="{\"event\":\"ffx_ps_block\",\"n\":";full+=std::to_string(n_row);char cell[32]{};
            sprintf_s(cell,",\"read\":[%d,%d,%d]",int(ok_ps),int(ok_light),int(ok_fog));full+=cell;
            const auto list=[&](const char* name,const uint32_t* v,size_t count){full+=",\"";full+=name;full+="\":\"";
                for(size_t i=0;i<count;++i){sprintf_s(cell,"%08X",v[i]);full+=cell;}full+="\"";};
            list("ps",ps.data(),ps.size());list("light",light.data(),light.size());list("fog",fog_block.data(),fog_block.size());full+="}\n";Log(full.c_str());}}
    {   // Snow witness, once: the draw context's +0x20 (the object 375DF0 reads the viewports from: +0x258, count +0x4D8) against the
        // native pass context and the effect contexts, and its +0x270..+0x27C (g_screenSize, g_recScreenSize).
        static std::atomic<bool> told{};bool expected=false;
        if(c && told.compare_exchange_strong(expected,true)){std::array<uintptr_t,6> w{};Read(c,w.data(),48);uintptr_t outer{};Read(c+0x100,&outer,8);
            std::array<float,4> screen{};Read(c+0x270,screen.data(),16);uint32_t count{};if(w[4])Read(w[4]+0x4D8,&count,4);
            char row[400]{};sprintf_s(row,"{\"event\":\"ffx_draw_context\",\"c\":\"%llX\",\"vt\":\"%llX\",\"w20\":\"%llX\",\"w28\":\"%llX\",\"outer\":\"%llX\",\"pass_context\":\"%llX\",\"inner\":\"%llX\",\"viewports\":%u,\"screen\":[%g,%g,%g,%g]}\n",
                static_cast<unsigned long long>(c),static_cast<unsigned long long>(w[0]>image?w[0]-image:w[0]),static_cast<unsigned long long>(w[4]),static_cast<unsigned long long>(w[5]),
                static_cast<unsigned long long>(outer),static_cast<unsigned long long>(native_ffx_draw?Context():0),static_cast<unsigned long long>([&]{uintptr_t i{};if(outer)Read(outer+8,&i,8);return i;}()),count,screen[0],screen[1],screen[2],screen[3]);Log(row);}}
    static const char* mode=nullptr;if(!mode)mode=native_ffx_draw?"native":"reference";
    {   // Distortion witness (FFX-DRAW, the distortion quads' rectangle): a draw whose t0 is the frame copy (+0x968 = +0xA70, slot 8
        // called with null): the FXR blend type +0x960, VS g_fbTexCoef (+0x2A0, slot 23), the PS image +0x4C0's g_color, the
        // wave words, g_vColorScale (+0x90; slot 13's scale DAT_14473E778 × +0xAA8), g_ps_ColorBlendType (+0x17C), softSpriteParam.
        uintptr_t t0{},copy{};static std::atomic<uint64_t> told{};
        if(c && Read(c+0x968,&t0,8) && Read(c+0xA70,&copy,8) && t0 && t0==copy){const auto k=told.fetch_add(1);
            uint32_t blend{};Read(c+0x960,&blend,4);static std::atomic<int> blended_rows{};   // I-0113 gap 13: every non-zero blend type too (the Normal 608s), 96 rows
            if(k<16 || k%2000==0 || (blend!=0 && blended_rows.fetch_add(1)<96)){int32_t color_type{};float global{},scale{};std::array<float,2> coef{};std::array<float,4> color{},colour_scale{},soft{};std::array<float,12> wave{};
                Read(c+0x2A0,coef.data(),8);Read(c+0x4C0,color.data(),16);Read(c+0x4C0+0x40,wave.data(),48);Read(c+0x4C0+0x90,colour_scale.data(),16);
                Read(c+0x4C0+0x17C,&color_type,4);Read(c+0x4C0+0x1D0,soft.data(),16);Read(image+0x473E778,&global,4);Read(c+0xAA8,&scale,4);
                // I-0113 tranche 30 (gap 16: natively some 0x2D draws blend with banks 0x10 and 0x20, never in the original): the draw
                // context's type +0x18 (264230 picks the blend from the bank by +0x960 for types 3 and 5, +0x08 always for type 1), the
                // blend slot 27 bound ([[c+0x100]+8]+0x4E8 as BankIndexOf's offset, -1 unknown), and +0xA39 (bit 0) with the global
                // 14496237D (bit 1), which force +0x960 to 0.
                int32_t type{-1},forced{};uintptr_t holder{},gx{},bound{};uint8_t a39{},global_flag{};Read(c+0x18,&type,4);
                if(Read(c+0x100,&holder,8) && holder && Read(holder+8,&gx,8) && gx)Read(gx+0x4E8,&bound,8);
                const int bank=ffx_draw_observe::BankIndexOf(bound);Read(c+0xA39,&a39,1);Read(image+0x496237D,&global_flag,1);forced=int(a39!=0)|int(global_flag!=0)<<1;
                char row[760]{};sprintf_s(row,"{\"event\":\"ffx_distortion_witness\",\"mode\":\"%s\",\"n\":%llu,\"blend\":%u,\"coef\":[%g,%g],\"color\":[%g,%g,%g,%g],\"near_wave\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g],"
                    "\"colour_scale\":[%g,%g,%g,%g],\"color_type\":%d,\"soft\":[%g,%g,%g,%g],\"global\":%g,\"aa8\":%g,\"type\":%d,\"bank\":%d,\"forced\":%d}\n",mode,static_cast<unsigned long long>(k),blend,coef[0],coef[1],
                    color[0],color[1],color[2],color[3],wave[0],wave[1],wave[2],wave[3],wave[4],wave[5],wave[6],wave[7],wave[8],wave[9],wave[10],wave[11],
                    colour_scale[0],colour_scale[1],colour_scale[2],colour_scale[3],color_type,soft[0],soft[1],soft[2],soft[3],global,scale,type,bank,forced);Log(row);}}}
    {   // Flame witness (FFX-DRAW, the whiter bonfire core): additive draws of pass type 3 (0x29): the PS image's colour scale (+0x90),
        // glow scale (+0x160), colour blend type (+0x17C), +0x1BC, soft-sprite words (+0x1D0), +0x220, and the b6 fog block (+0x880).
        uint32_t type{},blend{};static std::atomic<uint64_t> flames{};
        if(c && Read(c+0x18,&type,4) && type==3 && Read(c+0x960,&blend,4) && blend==2){const auto k=flames.fetch_add(1);
            if(k<24 || k%5000==0){std::array<float,4> scale{},glow{},soft{};std::array<int32_t,3> ints{};std::array<float,20> fog{};uintptr_t t0{};
                Read(c+0x4C0+0x90,scale.data(),16);Read(c+0x4C0+0x160,glow.data(),16);Read(c+0x4C0+0x17C,&ints[0],4);Read(c+0x4C0+0x1BC,&ints[1],4);
                Read(c+0x4C0+0x1D0,soft.data(),16);Read(c+0x4C0+0x220,&ints[2],4);Read(c+0x880,fog.data(),80);Read(c+0x968,&t0,8);
                std::string row="{\"event\":\"ffx_flame_witness\",\"mode\":\""+std::string(mode)+"\",\"n\":"+std::to_string(k);char cell[400]{};
                sprintf_s(cell,",\"t0\":\"%llX\",\"scale\":[%g,%g,%g,%g],\"glow\":[%g,%g,%g,%g],\"ints\":[%d,%d,%d],\"soft\":[%g,%g,%g,%g],\"fog\":[",static_cast<unsigned long long>(t0),
                    scale[0],scale[1],scale[2],scale[3],glow[0],glow[1],glow[2],glow[3],ints[0],ints[1],ints[2],soft[0],soft[1],soft[2],soft[3]);row+=cell;
                for(size_t i=0;i<fog.size();++i){char f[24]{};sprintf_s(f,"%s%.5g",i?",":"",fog[i]);row+=f;}
                row+="]}\n";Log(row.c_str());}}}
    uintptr_t holder{},vt{},getter{};uint64_t n{};bool table=false,qualified=false;std::array<uint32_t,4> words{};uint32_t count{},count2{},index{};
    if(c && Read(c+0xA60,&holder,8) && holder && Read(holder,&vt,8) && Read(vt+0x30,&getter,8) && getter){
        const auto object=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(getter)(holder);uintptr_t inner{},params{};
        if(object && Read(object+0x18,&inner,8) && inner){table=true;Read(inner+0x254,&count,4);Read(inner+0x25C,&count2,4);Read(inner+0x248,&index,4);
            qualified=count>0x1B && count2>6;
            if(qualified && Read(inner+0x260,&params,8) && params)for(size_t i=0;i<4;++i)Read(params+std::array<uint32_t,4>{0x50,0x58,0x64,0x68}[i]+4ull*index,&words[i],4);}}
    {const std::lock_guard hold(slot27_lock);n=++slot27_calls;slot27_tables+=table;slot27_qualified+=qualified;}
    if(n%3000!=1500)return;
    std::array<float,12> block{};Read(c+0x2D0,block.data(),32);Read(c+0x6A0,block.data()+8,16);std::array<float,4> w{};std::memcpy(w.data(),words.data(),16);
    char row[600]{};sprintf_s(row,"{\"event\":\"ffx_slot27_witness\",\"mode\":\"%s\",\"calls\":%llu,\"tables\":%llu,\"qualified\":%llu,\"count\":[%u,%u],\"index\":%u,\"words\":[%g,%g,%g,%g],\"vs_2d0\":[%g,%g,%g,%g,%g,%g,%g,%g],\"ps_6a0\":[%g,%g,%g,%g]}\n",
        mode,static_cast<unsigned long long>(n),static_cast<unsigned long long>(slot27_tables),static_cast<unsigned long long>(slot27_qualified),count,count2,index,w[0],w[1],w[2],w[3],
        block[0],block[1],block[2],block[3],block[4],block[5],block[6],block[7],block[8],block[9],block[10],block[11]);Log(row);}
// I-0105 tranche 10 (final review M1), Reference: 131780's blended `Snow` record, its five texture words (out +0x00..+0x20, the objects
// 103510 binds before the root's), its caller (23790, 103510, 10DAE0) and the blend, on each change of the words per caller (bounded).
using SnowBlend=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,float);SnowBlend snow_blend_original{};
void __fastcall SnowBlendObserved(uintptr_t out,uintptr_t a,uintptr_t b,float t) noexcept {
    snow_blend_original(out,a,b,t);
    static std::atomic<int> rows{};if(rows.load(std::memory_order_relaxed)>=32)return;
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;
    std::array<uintptr_t,5> words{};if(!Read(out,words.data(),sizeof(words)))return;
    static std::mutex lock;static std::map<uintptr_t,std::array<uintptr_t,5>> last;
    {const std::lock_guard hold(lock);const auto [at,fresh]=last.try_emplace(caller,words);if((!fresh && at->second==words) || rows>=32)return;at->second=words;++rows;}
    // Each record's name string (+0x28, ReadEngineWideString) and Texture:ID (+0x58).
    const auto name_of=[](uintptr_t record){std::string text;int32_t id{};
        for(const auto ch:ReadEngineWideString(record+0x28))text.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?static_cast<char>(ch):'?');
        Read(record+0x58,&id,4);return text+"#"+std::to_string(id);};
    const auto names=name_of(a)+"\",\""+name_of(b);
    char row[460]{};sprintf_s(row,"{\"event\":\"reference_snow_blend\",\"caller\":\"%llX\",\"records\":[\"%.200s\"],\"blend\":%.4g,\"words_00_08_10_18_20\":[\"%llX\",\"%llX\",\"%llX\",\"%llX\",\"%llX\"]}\n",
        static_cast<unsigned long long>(caller),names.c_str(),t,
        static_cast<unsigned long long>(words[0]),static_cast<unsigned long long>(words[1]),static_cast<unsigned long long>(words[2]),
        static_cast<unsigned long long>(words[3]),static_cast<unsigned long long>(words[4]));Log(row);}
// Snow witness, Reference (ffx-draw-observe.flag): the GX UpdateSubresource wrapper 1920300 (as effect_draw's native one): a
// VSCommonBuffer image and its writer.
using GxUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uintptr_t,uintptr_t,uint32_t,uint32_t);GxUpdate gx_update_original{};
std::mutex vs_common_lock;std::unordered_map<uintptr_t,uint64_t> vs_common_writers;
void __fastcall GxUpdateObserved(uintptr_t c,uintptr_t s,uintptr_t dst,uint32_t sub,uintptr_t box,uintptr_t src,uint32_t pitch,uint32_t slice) noexcept {
    const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-image;
    if(caller==0x5A0F2 && src)ffx_draw_observe::e03_instance_set=Read(src,ffx_draw_observe::e03_instance.data(),sizeof(ffx_draw_observe::e03_instance));   // 59470's instance block
    if(src && !box){std::array<float,0x58> f{};if(Read(src,f.data(),0x150) && f[0x53]>0 && f[0x53]<1 && f[0x52]>1000){uint64_t n{};
        {const std::lock_guard hold(vs_common_lock);n=++vs_common_writers[caller];}
        if(vs_image_rows_left.load()>0 && vs_image_rows_left.fetch_sub(1)>0){std::string vs="{\"event\":\"ffx_vs_image\",\"mode\":\"reference\",\"caller\":\"";char w[24]{};sprintf_s(w,"%llX\",\"f\":[",static_cast<unsigned long long>(caller));vs+=w;
            for(size_t k=0;k<0x150/4;++k){if(std::isfinite(f[k]))sprintf_s(w,"%s%.6g",k?",":"",f[k]);else sprintf_s(w,"%snull",k?",":"");vs+=w;}
            vs+="]}\n";Log(vs.c_str());}
        if(n==1 || n%5000==0){char row[300]{};sprintf_s(row,"{\"event\":\"ffx_vs_common\",\"mode\":\"reference\",\"caller\":\"%llX\",\"n\":%llu,\"w120\":[%g,%g,%g,%g,%g,%g,%g,%g]}\n",
            static_cast<unsigned long long>(caller),static_cast<unsigned long long>(n),f[0x48],f[0x49],f[0x4A],f[0x4B],f[0x4C],f[0x4D],f[0x4E],f[0x4F]);Log(row);}}}
    // I-0105 (the surface snow): 23790's cbPerFrame ([root+0x4DD0]+0x2B0) rows +0x60..+0xAF on each change, and 1A86C0's cbModelParam
    // ([root+0x4DD0]+0x2A0) FC_SnowRate +0x40: the draws with a non-zero rate counted, the first values and every change of value.
    if(src && !box){uintptr_t root{},buffers{},per_frame{},model_param{};
        if(Read(image+0x4961B50,&root,8) && root && Read(root+0x4DD0,&buffers,8) && buffers && Read(buffers+0x2B0,&per_frame,8) && Read(buffers+0x2A0,&model_param,8)){
            static std::mutex snow_lock;static std::array<float,20> last_rows{};static float last_rate{-1};static uint64_t rated{},updates{},rows_logged{},rates_logged{};
            if(dst==per_frame){std::array<float,20> rows{};if(Read(src+0x60,rows.data(),sizeof(rows))){const std::lock_guard hold(snow_lock);
                if(rows!=last_rows && rows_logged<64){last_rows=rows;++rows_logged;std::string row="{\"event\":\"reference_snow_rows\",\"rows\":[";
                    for(size_t i=0;i<rows.size();++i){char v[24]{};sprintf_s(v,"%s%.4g",i?",":"",rows[i]);row+=v;}row+="]}\n";Log(row.c_str());
                    // Tranche 10: 103510's fallbacks for t47..t51, the root's objects (+0x4800, +0x4810, +0x4818, +0x4808, +0x4820) and GX's
                    // default table [[144961AB0]+0x270] +0, +0x10, +0x20, +0x30: each object's table, so the native's records can be compared.
                    uintptr_t display{},defaults{};Read(image+0x4961AB0,&display,8);if(display)Read(display+0x270,&defaults,8);
                    const auto table_of=[](uintptr_t at){uintptr_t object{},table{};if(at && Read(at,&object,8) && object)Read(object,&table,8);
                        char v[48]{};sprintf_s(v,object?"\"%llX\"":"null",static_cast<unsigned long long>(table>image?table-image:0));return std::string(v);};
                    std::string objects="{\"event\":\"reference_snow_textures\",\"root\":[";
                    for(const uint32_t at:{0x4800u,0x4810u,0x4818u,0x4808u,0x4820u})objects+=(at==0x4800u?"":",")+table_of(root+at);
                    objects+="],\"defaults\":[";
                    for(const uint32_t at:{0u,0x10u,0x20u,0x30u})objects+=(at?",":"")+table_of(defaults?defaults+at:0);
                    objects+="]}\n";Log(objects.c_str());}}}
            else if(dst==model_param){float rate{};if(Read(src+0x40,&rate,4)){const std::lock_guard hold(snow_lock);++updates;if(rate!=0)++rated;
                if(updates%20000==0){char row[200]{};sprintf_s(row,"{\"event\":\"reference_snow_rate_count\",\"updates\":%llu,\"rated\":%llu}\n",
                    static_cast<unsigned long long>(updates),static_cast<unsigned long long>(rated));Log(row);}   // review L1: the counts past the cap
                if(rate!=last_rate && rates_logged<64){last_rate=rate;++rates_logged;char row[200]{};
                    sprintf_s(row,"{\"event\":\"reference_snow_rate\",\"rate\":%.4g,\"updates\":%llu,\"rated\":%llu}\n",rate,static_cast<unsigned long long>(updates),static_cast<unsigned long long>(rated));Log(row);}}}}}
    // I-0105 tracer witness: a whole update of a constant buffer (the wrapper hands [dst+0x18], the D3D11 resource, to the
    // context's UpdateSubresource; Reference only, so it is a genuine ID3D11Buffer).
    if(src && !box && !sub && ffx_draw_observe::TracerCollecting()){uintptr_t resource{};if(Read(dst+0x18,&resource,8) && resource){Microsoft::WRL::ComPtr<ID3D11Buffer> buffer;
        if(SUCCEEDED(reinterpret_cast<IUnknown*>(resource)->QueryInterface(IID_PPV_ARGS(&buffer)))){D3D11_BUFFER_DESC d{};buffer->GetDesc(&d);
            if(d.BindFlags&D3D11_BIND_CONSTANT_BUFFER)ffx_draw_observe::TracerBytes(dst,src,d.ByteWidth);}}}
    gx_update_original(c,s,dst,sub,box,src,pitch,slice);}
// I-0105 (Lightning Spear's tracer: natively every point of its strips at one place): the 606 renderer's (class 3D7E380) per-frame
// point append (vt+0x20, 373D00: each particle of the emitter param_3 through 1422EF7A0) and its history rebase (vt+0x30, 370620:
// every recorded point by the matrix param_4), in both modes (ffx-record.flag natively, ffx-draw-observe.flag in Reference): the
// calls, and for the first 6 of each and every 1200th the first particle's words and the first record's first point after it
// (append) or the matrix's 12 words (rebase).
using TracerAppend=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);using TracerRebase=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,const float*);
TracerAppend tracer_append_original{};TracerRebase tracer_rebase_original{};const char* tracer_mode="native";std::atomic<uint64_t> tracer_appends{},tracer_rebases{};
void TracerHistoryRow(const char* kind,uint64_t n,const float* f,size_t count) {
    std::string row="{\"event\":\"ffx_tracer_history\",\"mode\":\""+std::string(tracer_mode)+"\",\"kind\":\""+kind+"\",\"n\":"+std::to_string(n)+
        ",\"appends\":"+std::to_string(tracer_appends.load())+",\"rebases\":"+std::to_string(tracer_rebases.load())+",\"words\":[";
    for(size_t i=0;i<count;++i){char cell[32]{};if(std::isfinite(f[i]))sprintf_s(cell,"%s%.5g",i?",":"",f[i]);else sprintf_s(cell,"%snull",i?",":"");row+=cell;}
    row+="]}\n";Log(row.c_str());}
void __fastcall TracerAppendObserved(uintptr_t r,uintptr_t a2,uintptr_t emitter) noexcept {
    const auto n=++tracer_appends;uint32_t particles{};uintptr_t at{};
    if((n<=6 || n%1200==0) && Read(emitter+0x70,&particles,4) && particles && Read(emitter+0x60,&at,8)){std::array<float,10> f{};if(Read(at,f.data(),sizeof(f)))TracerHistoryRow("append",n,f.data(),f.size());}
    tracer_append_original(r,a2,emitter);
    uintptr_t records{},points{};std::array<float,9> p{};   // after: the first record's first point
    if((n<=6 || n%1200==0) && Read(r+0x20,&records,8) && records && Read(records,&points,8) && points && Read(points,p.data(),sizeof(p)))TracerHistoryRow("point0",n,p.data(),p.size());}
void __fastcall TracerRebaseObserved(uintptr_t r,uintptr_t a2,uintptr_t emitter,const float* m) noexcept {
    const auto n=++tracer_rebases;
    if((n<=6 || n%1200==0) && m){std::array<float,12> f{};if(Read(reinterpret_cast<uintptr_t>(m),f.data(),sizeof(f)))TracerHistoryRow("rebase",n,f.data(),f.size());}
    tracer_rebase_original(r,a2,emitter,m);}
void __fastcall SimulationObserved(uintptr_t s,uintptr_t c,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11) noexcept {
    SimulationWitness("reference",s,a7,a8,false);simulation_original(s,c,a3,a4,a5,a6,a7,a8,a9,a10,a11);SimulationWitness("reference",s,a7,a8,true);}
void __fastcall SimulationNative(uintptr_t s,uintptr_t c,uintptr_t a3,uintptr_t a4,uintptr_t a5,uintptr_t a6,uintptr_t a7,uintptr_t a8,uintptr_t a9,uintptr_t a10,uintptr_t a11) noexcept {
    std::array<uintptr_t,2> lent{};
    if(s)for(size_t i=0;i<2;++i){uintptr_t object{},vt{},view{};
        if(!Read(s+(i?0x37D0:0x2A10),&object,8) || !object || !Read(object,&vt,8) || vt!=image+0x3D1D528 || !Read(object+0x48,&view,8) || view)continue;
        if(!textures.Acquire(object)){++simulation_textures_unowned;continue;}
        *reinterpret_cast<uintptr_t*>(object+0x48)=effect_draw::TextureToken(object);lent[i]=object;++simulation_textures_lent;}
    static const bool witness=ModuleFlag(L"ffx-record.flag");static uint64_t calls{};
    if(witness && s && calls++%1200==600){std::string row="{\"event\":\"native_ffx_simulation_textures\",\"lent\":"+std::to_string(simulation_textures_lent.load())+",\"unowned\":"+std::to_string(simulation_textures_unowned.load());
        for(const uint32_t at:{0x2A10u,0x37D0u,0x24B8u,0x24D0u}){uintptr_t object{};Read(s+at,&object,8);char cell[64]{};sprintf_s(cell,",\"s%X\":{\"object\":\"%llX\"",at,static_cast<unsigned long long>(object));row+=cell;
            if(object && at!=0x24B8 && at!=0x24D0){std::array<uintptr_t,16> words{};Read(object,words.data(),sizeof(words));char vt[48]{};sprintf_s(vt,",\"vt\":\"%llX\",\"words\":[",static_cast<unsigned long long>(words[0]>image?words[0]-image:words[0]));row+=vt;
                for(size_t i=1;i<words.size();++i){char w[24]{};sprintf_s(w,"%s\"%llX\"",i>1?",":"",static_cast<unsigned long long>(words[i]));row+=w;}row+="]";}
            row+="}";}
        row+="}\n";Log(row.c_str());}
    if(witness)SimulationWitness("native",s,a7,a8,false);
    simulation_original(s,c,a3,a4,a5,a6,a7,a8,a9,a10,a11);
    if(witness)SimulationWitness("native",s,a7,a8,true);
    for(const auto object:lent)if(object)*reinterpret_cast<uintptr_t*>(object+0x48)=0;}
using Alloc=uintptr_t(__fastcall*)(uintptr_t,uint32_t,uint32_t);using Commit=void(__fastcall*)(uintptr_t);using BlockAlloc=uintptr_t(__fastcall*)(uintptr_t,uint64_t,uint64_t);
Alloc command_original{};Commit commit_original{};BlockAlloc block_original{};
uintptr_t __fastcall CommandAlloc(uintptr_t ctx,uint32_t type,uint32_t size) noexcept {
    if(ctx!=Context())return command_original(ctx,type,size);
    auto& c=commands.emplace_back();std::memcpy(&c.key,pass_context.data()+0x12C,4);std::memcpy(&c.pass,pass_context.data()+0x128,4);
    return reinterpret_cast<uintptr_t>(&c);   // the caller writes {callback, entity, data} (0x18 bytes: Command's first three words)
}
void __fastcall CommandCommit(uintptr_t ctx) noexcept {
    if(ctx!=Context()){commit_original(ctx);return;}
    pass_context[0x420]=0;pass_context[0x422]=0;pass_context[0x423]=0;std::memset(pass_context.data()+0x12C,0,4);   // BF2D0 with +0x421 set
}
uintptr_t __fastcall BlockAllocate(uintptr_t allocator,uint64_t size,uint64_t align) noexcept {
    if(allocator!=reinterpret_cast<uintptr_t>(sort_block.data())+0x38)return block_original(allocator,size,align);
    if(size>0x80)return 0;
    if(blocks_used==blocks.size())blocks.emplace_back();
    auto& b=blocks[blocks_used++];b.fill(0);return reinterpret_cast<uintptr_t>(b.data());
}
}
// FFX-DRAW N6, I-0104 (engine thread): the native effect context's statements as the renderer's NativeEffectFrame. Programmes: a token's GX shader object
// in the native registry (its bytes, keyed by the registry generation); the pixel programme's second target (SV_Target1) by
// reflection. Layout: the native record of 1925210 names the image's GX element table (28-byte records {semantic enum, index,
// DXGI format, slot, offset, class, step}, FFX-INPUT-LAYOUTS 1) and its count. States: the bank offsets of the state tokens.
// Textures t0..t4: an FFX texture token's GXTexture2D through TextureIngress (resident only), the default token SYSTEX_WHITE
// (INFERRED: the original's default is a white texture), the copy and depth tokens (the pass context's frame copy and depth
// objects) the scene-colour copy and the scene depth, a null view nothing. A draw missing a programme, layout or state is left out.
namespace native_ffx {
struct EffectFrameCounters {uint64_t frames{},draws{},dropped{},unresolved_textures{},default_textures{},foreign_buffers{},other_samplers{},gpu_statements{},copy_rects{},target_statements{},target_drops{};} effect_frame_counters;
std::shared_ptr<const NativeEffectLayout> EffectLayout(uintptr_t record) {
    static std::mutex lock;static std::unordered_map<uintptr_t,std::shared_ptr<const NativeEffectLayout>> layouts;
    uint32_t kind{};std::array<uint32_t,8> d{};if(!effect_system::Describe(record,kind,d) || kind!=4 || !d[0] || d[0]>32)return {};
    const auto table=uintptr_t(d[1])|(uintptr_t(d[2])<<32);
    if(table<image || table>=image+0x5000000)return {};
    {const std::lock_guard hold(lock);if(const auto found=layouts.find(table);found!=layouts.end())return found->second;}
    constexpr const char* names[]{"POSITION","BLENDWEIGHT","BLENDINDICES","NORMAL","PSIZE","TEXCOORD","TANGENT","BINORMAL","POSITIONT","COLOR"};
    auto layout=std::make_shared<NativeEffectLayout>();layout->key=table;
    for(uint32_t i=0;i<d[0];++i){std::array<uint32_t,7> e{};if(!Read(table+28ull*i,e.data(),28) || e[0]>9 || e[3] || e[5])return {};
        layout->elements.push_back({names[e[0]],e[1],static_cast<DXGI_FORMAT>(e[2]),e[4]});}
    const std::lock_guard hold(lock);return layouts.emplace(table,std::move(layout)).first->second;}
NativeEffectProgram EffectProgram(uintptr_t object,uint32_t* targets=nullptr) {
    const auto* token=effect_draw::AsToken(object);if(!token || token->magic!=effect_draw::program_magic)return {};
    std::shared_ptr<const NativeShader> shader;{const std::lock_guard hold(shader_lock);const auto found=shaders.find(token->payload);if(found==shaders.end())return {};shader=found->second;}
    if(targets){   // SV_Target1 written: two targets (cached by registry generation)
        static std::mutex lock;static std::unordered_map<uint64_t,uint32_t> outputs;
        {const std::lock_guard hold(lock);if(const auto found=outputs.find(shader->generation);found!=outputs.end()){*targets=found->second;}}
        if(!*targets){uint32_t count=1;Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;D3D11_SHADER_DESC desc{};
            if(SUCCEEDED(D3DReflect(shader->bytes.data(),shader->bytes.size(),IID_PPV_ARGS(&reflection))) && SUCCEEDED(reflection->GetDesc(&desc)))
                for(UINT i=0;i<desc.OutputParameters;++i){D3D11_SIGNATURE_PARAMETER_DESC p{};if(SUCCEEDED(reflection->GetOutputParameterDesc(i,&p)) && p.SystemValueType==D3D_NAME_TARGET && p.SemanticIndex==1)count=2;}
            const std::lock_guard hold(lock);outputs[shader->generation]=count;*targets=count;}}
    return {shader->generation,std::shared_ptr<const std::vector<uint8_t>>(shader,&shader->bytes)};}
uint32_t BankOffset(uintptr_t object) {const auto* t=effect_draw::AsToken(object);return t && t->magic==effect_draw::state_magic?t->value:0;}
// I-0116 tranche 1 (UPSCALER-DLSS-READY §1.2, `projection-vars.tsv`): the projections an effect programme rasterises with, by
// reflection: in its constant buffers bound where the effect context gives a block (VS b0, DS b0, GS b9, PS b0, b1, b6, b9, b11),
// a used 64-byte rasterising matrix (`ffx-program-layouts.tsv`, every stage): the pure projections `g_projectionMatrix` (the
// GXFfx vertex programmes' VSCommonBuffer, offset 128) and `g_ds_projectionMatrix` (the tessellated ones' DSCommonBuffer, 128),
// their layout told by their w vector; the world-to-clip ones (`g_viewProjectionMatrix`, `g_ds_viewProjectionMatrix`, the
// `…worldViewProjection…` matrices, the GPU particles' `m_gViewProjMatrix` at 208 of gcbSceneParam), their layout told at each
// frame by upscaler::WorldClipLayout (a view-projection against the scene camera's, a world-view-projection by its own
// perspective w; second step review M-a, fourth L4); each as {its block's
// slot, its byte offset, its kind, its declared class}; cached by registry generation. Not the inverses, nor the compute sort's
// matrices.
// Their blocks are the game's, unjittered (I-0111's known limit): the frame's copy takes the scene camera's jitter.
struct EffectProjectionSite {uint32_t slot{};upscaler::ProjectionSite site;};
// Returned by reference: the cache's values stay where they are (a node map; final review L3).
const std::vector<EffectProjectionSite>& EffectProjectionSites(const NativeEffectProgram& program,D3D11_SHADER_VERSION_TYPE stage) {
    static const std::vector<EffectProjectionSite> none;if(!program.bytes || program.bytes->empty())return none;
    static std::mutex lock;static std::unordered_map<uint64_t,std::vector<EffectProjectionSite>> sites;
    const uint64_t key=program.key^(uint64_t(stage)<<60);
    {const std::lock_guard hold(lock);if(const auto found=sites.find(key);found!=sites.end())return found->second;}
    std::vector<EffectProjectionSite> found;Microsoft::WRL::ComPtr<ID3D11ShaderReflection> reflection;D3D11_SHADER_DESC desc{};
    const auto slot=[stage](UINT bind)->int{
        switch(stage){case D3D11_SHVER_VERTEX_SHADER:return bind==0?EffectVsB0:-1;case D3D11_SHVER_DOMAIN_SHADER:return bind==0?EffectDsB0:-1;
            case D3D11_SHVER_GEOMETRY_SHADER:return bind==9?EffectGsB9:-1;
            case D3D11_SHVER_PIXEL_SHADER:return bind==0?EffectPsB0:bind==1?EffectPsB1:bind==6?EffectPsB6:bind==9?EffectPsB9:bind==11?EffectPsB11:-1;
            default:return -1;}};
    if(SUCCEEDED(D3DReflect(program.bytes->data(),program.bytes->size(),IID_PPV_ARGS(&reflection))) && SUCCEEDED(reflection->GetDesc(&desc)))
        for(UINT b=0;b<desc.ConstantBuffers;++b){auto* buffer=reflection->GetConstantBufferByIndex(b);D3D11_SHADER_BUFFER_DESC bd{};D3D11_SHADER_INPUT_BIND_DESC bind{};
            if(!buffer || FAILED(buffer->GetDesc(&bd)) || bd.Type!=D3D_CT_CBUFFER || FAILED(reflection->GetResourceBindingDescByName(bd.Name,&bind)))continue;
            const int s=slot(bind.BindPoint);if(s<0)continue;
            for(UINT v=0;v<bd.Variables;++v){D3D11_SHADER_VARIABLE_DESC vd{};auto* variable=buffer->GetVariableByIndex(v);
                if(!variable || FAILED(variable->GetDesc(&vd)) || !vd.Name || vd.Size!=64 || !(vd.uFlags&D3D_SVF_USED))continue;
                D3D11_SHADER_TYPE_DESC td{};auto* type=variable->GetType();if(!type || FAILED(type->GetDesc(&td)))continue;
                const int declared=td.Class==D3D_SVC_MATRIX_COLUMNS?1:td.Class==D3D_SVC_MATRIX_ROWS?2:0;if(!declared)continue;
                const std::string_view name(vd.Name);
                const int kind=name=="g_projectionMatrix" || name=="g_ds_projectionMatrix"?0:
                    name=="g_viewProjectionMatrix" || name=="g_ds_viewProjectionMatrix" || name=="m_gViewProjMatrix"?1:
                    name=="g_worldViewProjectionMatrix" || name=="g_ds_worldViewProjectionMatrix" || name=="m_fWorldViewProjection"?2:-1;
                if(kind>=0)found.push_back({uint32_t(s),{vd.StartOffset,kind,declared}});}}
    const std::lock_guard hold(lock);return sites.emplace(key,std::move(found)).first->second;}
struct EffectJitterCounters {uint64_t frames{},bare{};upscaler::JitterCounts sites;} effect_jitter_counters;
uint32_t EffectTopology(uint32_t gx) {return gx>=1 && gx<=5?gx:gx>=6 && gx<=9?gx+4:gx>=10 && gx<=41?gx+23:0;}   // 1929BA0
// I-0104 (DESIGN §2): the native effect context's emit. Each statement the context hands over (effect_draw::Execute) becomes the
// renderer's NativeEffectDraw at once, in order: no captured record is translated afterwards. Its constant blocks and the ring's
// vertices are ranges of the context's arena, which becomes the frame's arena at Finish.
static_assert(effect_draw::zero_bytes==effect_arena_zero,"the arena's zero block: one size on both sides");
class EffectFrameBuilder {
public:
    EffectFrameBuilder():frame_(new NativeEffectFrame,&Drop) {
        ++effect_frame_counters.frames;
        static const bool frame_witness=ModuleFlag(L"ffx-record.flag");frame_->witness=frame_witness;
        static const bool frame_measure=ModuleFlag(L"effect-measure.flag");frame_->measure=frame_measure;   // I-0127: the stage's measure
        static const bool frame_scan=ModuleFlag(L"effect-scan.flag");frame_->scan=frame_scan;   // I-0113 tranche 29: the non-finite scan
        static const bool frame_scan_draws=ModuleFlag(L"effect-scan-draws.flag");frame_->scan_draws=frame_scan_draws;   // I-0113 tranche 30: per statement
        static const bool no_effect_copy=ModuleFlag(L"no-ffx-effect-copy.flag");frame_->effect_copy=!no_effect_copy;
        // I-0113 tranche 17: pass 0x31 into its half-size layer (no-effect-layer.flag: at full size into the scene, as before; A/B witness).
        static const bool no_effect_layer=ModuleFlag(L"no-effect-layer.flag");frame_->effect_layer=!no_effect_layer;
        // Its composite (140102790's block byte 0): bilateral when the Effects quality row's « Force HQ UpScale » is set
        // ([[GXRS+0x5360]+0x4B], STUDY-T17-LAYER-ALPHA §5b: Effects MAX only), from the frame's option snapshot (O2: the
        // quality object as written for its levels; final review M1: not the live block).
        static_assert(bootstrap::QualityObjectBytes>0x4B);
        {const auto options=AcquireNativeOptions();frame_->layer_bilateral=options && options->object_valid && options->object[0x4B]!=0;}
        // Before Execute, as main's order (final review S1): a record released during Execute has no serial left and is never
        // described, so its release reaches the renderer with the next frame and nothing is made for it.
        {const std::lock_guard hold(taken_lock_);sent_.insert(taken_.begin(),taken_.end());taken_.clear();}
        frame_->released=effect_draw::TakeReleased();
        for(const uint64_t key:frame_->released)sent_.erase(key);}
    void Statement(const effect_draw::EffectDrawRecord& r);
    // I-0116 tranche 1: with the scene camera jittered (DLSS), each projection the frame's draws rasterise with (EffectProjectionSites)
    // takes the camera's NDC shift in the frame's copy of its block, once a block (upscaler::ShiftSite); a world-to-clip block by
    // the layout upscaler::WorldClipLayout tells (a view-projection's against `vp`, the scene camera's unjittered one). A draw whose last geometry stage (GS, else DS, else VS)
    // has no site is counted bare (second step review M-b); the programmes of the first four bare draws and of the first four
    // sites left (refused, outside, unmatched: final review L1) are named, so a family cannot drop out unseen. Capped production
    // rows, as native_dlss_frames.
    std::shared_ptr<const NativeEffectFrame> Finish(const std::array<float,2>& ndc,bool jittered,const VelocityMatrix* vp) {
        const auto& arena=effect_draw::Arena();
        frame_->arena.assign(arena.begin(),arena.begin()+static_cast<ptrdiff_t>(std::min(effect_draw::ArenaUsed(),arena.size())));
        frame_->target_size=effect_draw::TargetSize();   // I-0140 tranche 10
        if(jittered){auto& c=effect_jitter_counters;++c.frames;std::unordered_set<uint64_t> done;
            VelocityMatrix inverse{};const bool inverted=vp && VelocityInverse(*vp,inverse);
            // A programme named once by its pass, stage and its constant buffers' used variables.
            const auto name=[](std::unordered_set<uint64_t>& named,const char* event,const char* why,const NativeEffectProgram& program,const char* stage,int pass){
                if(named.size()>=4 || !program.bytes || !named.insert(program.key).second)return;
                std::string vars;Microsoft::WRL::ComPtr<ID3D11ShaderReflection> r;D3D11_SHADER_DESC sd{};
                if(SUCCEEDED(D3DReflect(program.bytes->data(),program.bytes->size(),IID_PPV_ARGS(&r))) && SUCCEEDED(r->GetDesc(&sd)))
                    for(UINT b=0;b<sd.ConstantBuffers && vars.size()<400;++b){auto* cb=r->GetConstantBufferByIndex(b);D3D11_SHADER_BUFFER_DESC bd{};if(!cb || FAILED(cb->GetDesc(&bd)))continue;
                        for(UINT v=0;v<bd.Variables && vars.size()<400;++v){D3D11_SHADER_VARIABLE_DESC vd{};auto* var=cb->GetVariableByIndex(v);
                            if(var && SUCCEEDED(var->GetDesc(&vd)) && vd.Name && (vd.uFlags&D3D_SVF_USED)){if(!vars.empty())vars+=',';vars+=vd.Name;}}}
                const std::string row=std::string("{\"event\":\"")+event+"\",\"why\":\""+why+"\",\"pass\":"+std::to_string(pass)+",\"stage\":\""+stage+
                    "\",\"bytes\":"+std::to_string(program.bytes->size())+",\"used\":\""+vars+"\"}\n";Log(row.c_str());};
            static std::unordered_set<uint64_t> named_bare,named_left;
            for(const auto& d:frame_->draws){
                if(d.op!=NativeEffectOp::Draw && d.op!=NativeEffectOp::DrawIndirect)continue;
                const auto* last=d.gs.bytes?&d.gs:d.ds.bytes?&d.ds:&d.vs;bool sited{};
                for(const auto& [program,stage,stage_name]:{std::tuple{&d.vs,D3D11_SHVER_VERTEX_SHADER,"vs"},{&d.ds,D3D11_SHVER_DOMAIN_SHADER,"ds"},
                        {&d.gs,D3D11_SHVER_GEOMETRY_SHADER,"gs"},{&d.ps,D3D11_SHVER_PIXEL_SHADER,"ps"}})
                    for(const auto& site:EffectProjectionSites(*program,stage)){
                        if(program==last)sited=true;const auto& range=d.constants[site.slot];
                        const auto kind=size_t(site.site.kind);const auto before=c.sites;
                        const int layout=upscaler::ShiftSite(frame_->arena.data(),frame_->arena.size(),range.offset,range.size,site.site,ndc,inverted?&inverse:nullptr,done,c.sites);
                        if(kind && !before.shifted[kind] && layout){char row[200]{};   // the first view-projection and world-view-projection told by the camera
                            sprintf_s(row,"{\"event\":\"native_ffx_jitter_world\",\"kind\":%zu,\"slot\":%u,\"offset\":%u,\"layout\":%d,\"declared\":%d,\"pass\":%d}\n",
                                kind,site.slot,site.site.offset,layout,site.site.declared,d.pass);Log(row);}
                        const char* why=c.sites.refused!=before.refused?"refused":c.sites.outside!=before.outside?"outside":c.sites.unmatched!=before.unmatched?"unmatched":nullptr;
                        if(why)name(named_left,"native_ffx_jitter_left",why,*program,stage_name,d.pass);}
                if(!sited){++c.bare;name(named_bare,"native_ffx_jitter_bare","no site",*last,last==&d.gs?"gs":last==&d.ds?"ds":"vs",d.pass);}}
            if(c.frames==1 || !(c.frames%3600)){const auto& n=c.sites;char row[320]{};
                sprintf_s(row,"{\"event\":\"native_ffx_jitter\",\"frames\":%llu,\"shifted\":[%llu,%llu,%llu],\"refused\":%llu,\"outside\":%llu,\"disagreed\":%llu,\"unmatched\":%llu,\"bare\":%llu}\n",
                static_cast<unsigned long long>(c.frames),static_cast<unsigned long long>(n.shifted[0]),static_cast<unsigned long long>(n.shifted[1]),static_cast<unsigned long long>(n.shifted[2]),static_cast<unsigned long long>(n.refused),static_cast<unsigned long long>(n.outside),
                static_cast<unsigned long long>(n.disagreed),static_cast<unsigned long long>(n.unmatched),static_cast<unsigned long long>(c.bare));Log(row);}}
        return frame_;}
private:
    static NativeEffectRange Range(const effect_draw::Range& r) noexcept {return {r.offset,r.size};}
    // A constant block's bytes in the context's arena (the witnesses).
    static std::pair<const uint8_t*,size_t> Bytes(const NativeEffectRange& r) noexcept {
        const auto& arena=effect_draw::Arena();if(!r.size || uint64_t(r.offset)+r.size>arena.size())return {nullptr,0};return {arena.data()+r.offset,r.size};}
    // E07: the effect records the statements reach, described once a frame (a view with its buffer); a buffer's creation bytes go
    // with its descriptions until a frame the stage took carried them (the renderer keeps the resource; sent_ records what it took).
    uint64_t Object(uintptr_t handle) {
        uint32_t kind{};std::array<uint32_t,8> d{};if(!handle || !effect_system::Describe(handle,kind,d) || kind<1 || kind>3)return 0;
        const auto key=effect_system::Serial(handle);if(!key)return 0;
        if(listed_.insert(key).second){NativeEffectObject o;o.key=key;o.kind=kind;o.description=d;
            if(kind!=1)o.parent=Object(effect_system::ViewParent(handle));
            else if(!sent_.contains(key)){auto bytes=std::make_shared<std::vector<uint8_t>>();effect_system::BufferBytes(handle,*bytes);o.initial=std::move(bytes);}
            frame_->objects.push_back(std::move(o));}
        return key;}
    // A bound view: an FFX texture (TextureIngress), the copy or depth token, an effect record's view (E07), or nothing. False
    // (I-0120 D2) when an FFX texture or SYSTEX_WHITE is not resident yet: the draw is dropped for that frame, never drawn with a
    // null view (D-0197); an unknown handle stays counted only.
    bool Resolve(uintptr_t bound,NativeEffectTexture& out) {
        if(!bound)return true;
        const auto* token=effect_draw::AsToken(bound);
        if(token && token->magic==effect_draw::copy_magic){out.kind=NativeEffectTextureKind::SceneCopy;return true;}
        if(token && token->magic==effect_draw::depth_magic){out.kind=NativeEffectTextureKind::Depth;return true;}
        // I-0140 tranche 10: a half-size effect target's shader view (271B40 binds them for its blur and its composite).
        if(token && token->magic==effect_draw::target_magic){const uint32_t index=token->value>>2;
            if((token->value&3)!=uint32_t(effect_draw::TargetRole::Srv) || !index || index>=effect_draw::effect_targets){++effect_frame_counters.target_drops;return false;}
            out.kind=NativeEffectTextureKind::Target;out.object=index;return true;}
        if(token && token->magic==effect_draw::texture_magic){const auto record=textures.Acquire(token->payload);
            if(record && record->resident && record->view){out.kind=NativeEffectTextureKind::Texture;out.view=record->view.handle;frame_->owners.push_back(record);return true;}
            ++effect_frame_counters.unresolved_textures;return false;}
        if(token && token->magic==effect_draw::default_magic){   // the GX solid texture [[display+0x270]]: INFERRED white, SYSTEX_WHITE
            ++effect_frame_counters.default_textures;
            if(const auto common=AcquireNativeCommonImages())for(const auto& [system_name,record]:common->system)
                if(system_name==L"SYSTEX_WHITE" && record && record->resident && record->view){out.kind=NativeEffectTextureKind::Texture;out.view=record->view.handle;frame_->owners.push_back(record);return true;}
            ++effect_frame_counters.unresolved_textures;return false;}
        if(const auto key=Object(bound)){out.kind=NativeEffectTextureKind::Object;out.object=key;return true;}
        ++effect_frame_counters.unresolved_textures;return true;}
    // The vertices and indices: the ring's (offsets into the arena) or a native effect buffer with its creation bytes (the triangle
    // builder 396F30's static quad, Native1970), appended to the arena once a frame; anything else drops the draw.
    bool Place(uintptr_t buffer,uint32_t offset,uint32_t& placed) {
        if(const auto* t=effect_draw::AsToken(buffer);t && t->magic==effect_draw::ring_magic){placed=offset;return true;}
        auto found=static_buffers_.find(buffer);
        if(found==static_buffers_.end()){std::vector<uint8_t> bytes;if(!effect_system::BufferBytes(buffer,bytes) || bytes.empty())return false;
            const auto at=effect_draw::Append(bytes.data(),bytes.size(),16);if(at==UINT32_MAX)return false;
            found=static_buffers_.emplace(buffer,at).first;}
        placed=found->second+offset;return true;}
    void Gpu(const effect_draw::EffectDrawRecord& r,NativeEffectDraw& d);
    std::shared_ptr<NativeEffectFrame> frame_;std::unordered_map<uintptr_t,uint32_t> static_buffers_;std::unordered_set<uint64_t> listed_;
    // I-0120 D1 (the final review's L3): the buffers whose creation bytes a frame the stage took carried; every frame carries them
    // until then (the few frames in flight), so a dropped frame never leaves a buffer without them. A frame's deleter (its last
    // holder, on any thread) reports what it carried when the stage took it (`taken_`), or gives its releases back when the stage
    // never took it (Execute threw, its scene not prepared, a render-extent change, the DLSS latch, Encode stopped first).
    static inline std::unordered_set<uint64_t> sent_;
    static inline std::mutex taken_lock_;static inline std::vector<uint64_t> taken_;
    static void Drop(NativeEffectFrame* frame) noexcept {
        try{if(!frame->consumed.load(std::memory_order_acquire))effect_draw::GiveBack(frame->released);
            else{const std::lock_guard hold(taken_lock_);for(const auto& o:frame->objects)if(o.kind==1 && o.initial)taken_.push_back(o.key);}}catch(...){}
        delete frame;}
};
// I-0113 tranche 17 witness (ffx-record.flag): the blend bank offsets the effect draws carry, by engine pass and draw kind
// (native_effect_pass_blends, every 3000th frame), to compare with the original's reference_effect_pass_blends.
// I-0140: the depth entry too for passes 0x20 and 0x21 (STUDY-EFFECT-PASSES-20-21 reads 0x21 as opaque, depth written, B+0x08 and
// B+0x1E0; i140-e5a saw every 0x21 draw bind them, and no 0x20 draw), whose depth write the stage keeps for 0x21.
void EffectPassBlendWitness(int32_t pass,uint32_t blend,bool particles,uint32_t targets,uint32_t depth=0) {
    static const bool witness=ModuleFlag(L"ffx-record.flag");if(!witness)return;
    static std::mutex blends_lock;static std::map<std::string,uint64_t> uses;static uint64_t last_frame{},frames_seen{};
    const std::lock_guard hold(blends_lock);
    char key[64]{};if(pass==0x20 || pass==0x21)sprintf_s(key,"%X/%s/%X/rt%u/d%X",unsigned(pass),particles?"gpu":"draw",blend,targets,depth);
    else sprintf_s(key,"%X/%s/%X/rt%u",unsigned(pass),particles?"gpu":"draw",blend,targets);++uses[key];
    if(last_frame==effect_frame_counters.frames)return;last_frame=effect_frame_counters.frames;
    if(++frames_seen%3000!=1500)return;
    std::string row="{\"event\":\"native_effect_pass_blends\",\"frames\":"+std::to_string(frames_seen)+",\"uses\":{";
    bool first=true;for(const auto& [k,v]:uses){row+=(first?"\"":",\"")+k+"\":"+std::to_string(v);first=false;}row+="}}\n";Log(row.c_str());}
// E07: the GPU-particle statements (compute, copy, clear, the particles' indirect draw), in their place in the stream.
void EffectFrameBuilder::Gpu(const effect_draw::EffectDrawRecord& r,NativeEffectDraw& d) {
    auto& counters=effect_frame_counters;
    using O=effect_draw::Op;
    d.op=r.op==O::Dispatch?NativeEffectOp::Dispatch:r.op==O::DispatchIndirect?NativeEffectOp::DispatchIndirect:r.op==O::Copy?NativeEffectOp::Copy:
        r.op==O::ClearUav?NativeEffectOp::ClearUav:r.op==O::CopyRect?NativeEffectOp::CopyRect:r.op==O::ClearTarget?NativeEffectOp::ClearTarget:
        r.op==O::CopyTarget?NativeEffectOp::CopyTarget:NativeEffectOp::DrawIndirect;
    // I-0140 tranche 10: a half-size effect target's clear or whole copy (the light shafts' 271B40), in its place in the stream.
    if(d.op==NativeEffectOp::ClearTarget){d.target=r.clear_target;d.clear_colour=r.clear_colour;frame_->draws.push_back(std::move(d));++counters.target_statements;return;}
    if(d.op==NativeEffectOp::CopyTarget){d.target=r.copy_target_destination;d.copy_source=r.copy_target_source;frame_->draws.push_back(std::move(d));++counters.target_statements;return;}
    if(d.op==NativeEffectOp::CopyRect){d.copy_rect=r.copy_rect;d.copy_kind=r.copy_kind;d.copy_size=r.copy_size;   // I-0113 tranche 23
        // Tranche 31 (ffx-series.flag): inside the series window each quad copy's rectangle (the target's fraction; the renderer
        // floors, ceils and widens it by 16 pixels), against the original's region copies (ffx_distortion_series_copy).
        static const bool series=ModuleFlag(L"ffx-series.flag");static std::atomic<int> copy_rows{};
        if(series){const double ms=ffx_draw_observe::SeriesMs();
            if(ms>=0 && copy_rows.fetch_add(1)<4000){char row[300]{};std::string rect;   // the step review's L1: a rectangle not finite written null
                for(size_t i=0;i<4;++i){char cell[32]{};if(std::isfinite(d.copy_rect[i]))sprintf_s(cell,"%s%.6g",i?",":"",double(d.copy_rect[i]));else sprintf_s(cell,"%snull",i?",":"");rect+=cell;}
                sprintf_s(row,"{\"event\":\"ffx_distortion_series_copyrect\",\"ms\":%.3f,\"pass\":%d,\"key\":%u,\"kind\":%u,\"rect\":[%s],\"size\":[%d,%d]}\n",ms,int(d.pass),d.key,unsigned(d.copy_kind),
                    rect.c_str(),d.copy_size[0],d.copy_size[1]);Log(row);}}
        frame_->draws.push_back(std::move(d));++counters.copy_rects;return;}
    if(r.constants_invalid){++counters.dropped;return;}
    for(uint32_t c=0;c<EffectConstantSlots;++c)d.constants[c]=Range(r.constants[c]);
    const auto floats=[&](std::string& row,const char* name,uint32_t slot,size_t limit){row+=",\"";row+=name;row+="\":[";const auto [data,size]=Bytes(d.constants[slot]);
        for(size_t i=0;i*4<size && i<limit;++i){float v{};std::memcpy(&v,data+i*4,4);char cell[24]{};sprintf_s(cell,"%s%.4g",i?",":"",v);row+=cell;}row+="]";};
    if(d.op==NativeEffectOp::Dispatch || d.op==NativeEffectOp::DispatchIndirect){
        d.cs=EffectProgram(r.cs);if(!d.cs.bytes){++counters.dropped;return;}
        for(uint32_t t=0;t<5;++t)if(!Resolve(r.cs_resources[t],d.cs_resources[t])){++counters.dropped;return;}   // I-0120 D2
        for(uint32_t u=0;u<3;++u)d.cs_uavs[u]=Object(r.cs_uavs[u]);
        for(uint32_t u=0;u<3;++u)d.cs_uav_counts[u]=r.cs_uav_counts[u];d.cs_uav_counts_set=static_cast<uint8_t>(r.cs_uav_counts_set&7);
        std::copy(std::begin(r.groups),std::end(r.groups),std::begin(d.groups));
        if(d.op==NativeEffectOp::DispatchIndirect && !(d.args=Object(r.args))){++counters.dropped;return;}
        // E07 witness (ffx-record.flag): the dispatch's resources (bound value: token magic, effect kind or engine object; resolved kind),
        // UAV counts and CS b0 / b5, every 1200th indirect dispatch.
        static const bool witness=ModuleFlag(L"ffx-record.flag");static uint64_t dispatches{};
        if(witness && d.op==NativeEffectOp::DispatchIndirect && dispatches++%1200==600){
            std::string row="{\"event\":\"native_ffx_particle_dispatch\",\"t\":[";
            for(uint32_t t=0;t<5;++t){const auto bound=r.cs_resources[t];uint32_t kind{};std::array<uint32_t,8> desc{};char cell[160]{};
                const auto* token=effect_draw::AsToken(bound);
                if(!bound)sprintf_s(cell,"%s\"null\"",t?",":"");
                else if(token)sprintf_s(cell,"%s\"token %08X res %d\"",t?",":"",token->magic,int(d.cs_resources[t].kind));
                else if(effect_system::Describe(bound,kind,desc))sprintf_s(cell,"%s\"effect %u [%u,%u,%u,%u] res %d\"",t?",":"",kind,desc[0],desc[1],desc[2],desc[3],int(d.cs_resources[t].kind));
                else{uintptr_t vt{};Read(bound,&vt,8);sprintf_s(cell,"%s\"engine vt %llX res %d\"",t?",":"",static_cast<unsigned long long>(vt?vt-image:0),int(d.cs_resources[t].kind));}
                row+=cell;}
            char counts[120]{};sprintf_s(counts,"],\"counts\":[%u,%u,%u],\"counts_set\":%u",r.cs_uav_counts[0],r.cs_uav_counts[1],r.cs_uav_counts[2],unsigned(d.cs_uav_counts_set));row+=counts;
            floats(row,"cs_b0",EffectCsB0,32);floats(row,"cs_b1",EffectCsB1,8);floats(row,"cs_b5",EffectCsB5,16);row+="}\n";Log(row.c_str());}}
    else if(d.op==NativeEffectOp::Copy){d.destination=Object(r.destination);d.source=Object(r.source);if(!d.destination || !d.source){++counters.dropped;return;}}
    else if(d.op==NativeEffectOp::ClearUav){d.clear_view=Object(r.clear_view);d.clear_values=r.clear_values;if(!d.clear_view){++counters.dropped;return;}}
    else{   // the particles' indirect draw: VS + GS + PS, points, VS t0/t1, the bank's states
        if(r.target){++counters.target_drops;++counters.dropped;return;}   // I-0140 tranche 10: drawn into the pass's own target only
        d.vs=EffectProgram(r.vs);d.gs=EffectProgram(r.gs);uint32_t targets{};d.ps=EffectProgram(r.ps,&targets);d.targets=targets?targets:1;
        d.topology=EffectTopology(r.topology);d.blend=BankOffset(r.blend);d.depth=BankOffset(r.depth);d.raster=BankOffset(r.raster);
        if(!d.vs.bytes || !d.gs.bytes || !d.ps.bytes || !d.blend || !d.depth || !d.raster || !(d.args=Object(r.args))){++counters.dropped;return;}
        EffectPassBlendWitness(d.pass,d.blend,true,d.targets,d.depth);
        for(uint32_t t=0;t<2;++t)if(!Resolve(r.vs_resources[t],d.vs_resources[t])){++counters.dropped;return;}
        for(uint32_t t=0;t<5;++t)if(!Resolve(r.ps_resources[t],d.textures[t])){++counters.dropped;return;}
        // E07 witness (ffx-record.flag): the particle draw's constant blocks and bindings, every 1200th draw.
        static const bool witness=ModuleFlag(L"ffx-record.flag");static uint64_t particle_draws{};
        if(witness && particle_draws++%1200==600){
            std::string row="{\"event\":\"native_ffx_particle_draw\",\"sizes\":[";
            for(uint32_t c=0;c<EffectConstantSlots;++c){char cell[16]{};sprintf_s(cell,"%s%u",c?",":"",d.constants[c].size);row+=cell;}
            row+="]";
            floats(row,"gs_b9",EffectGsB9,112);floats(row,"ps_b0",EffectPsB0,140);floats(row,"ps_b6",EffectPsB6,16);floats(row,"ps_b9",EffectPsB9,16);floats(row,"ps_b11",EffectPsB11,16);
            char tail[200]{};sprintf_s(tail,",\"vs_t\":[%d,%d],\"ps_t\":[%d,%d,%d,%d],\"blend\":\"%X\",\"depth\":\"%X\",\"raster\":\"%X\",\"targets\":%u}\n",
                int(d.vs_resources[0].kind),int(d.vs_resources[1].kind),int(d.textures[0].kind),int(d.textures[1].kind),int(d.textures[2].kind),int(d.textures[3].kind),
                d.blend,d.depth,d.raster,d.targets);
            row+=tail;Log(row.c_str());}}
    frame_->draws.push_back(std::move(d));++counters.gpu_statements;}
void EffectFrameBuilder::Statement(const effect_draw::EffectDrawRecord& r) {
    auto& counters=effect_frame_counters;
    NativeEffectDraw d;d.pass=r.pass;d.key=r.key;
    if(r.op!=effect_draw::Op::Draw){Gpu(r,d);return;}
    d.topology=EffectTopology(r.topology);
    d.vs=EffectProgram(r.vs);d.hs=EffectProgram(r.hs);d.ds=EffectProgram(r.ds);uint32_t targets{};d.ps=EffectProgram(r.ps,&targets);d.targets=targets?targets:1;
    d.layout=EffectLayout(r.layout);
    d.blend=BankOffset(r.blend);d.depth=BankOffset(r.depth);d.raster=BankOffset(r.raster);d.sampler=BankOffset(r.ps_samplers[9]);
    if(!d.topology || !d.vs.bytes || !d.ps.bytes || !d.layout || !d.blend || !d.depth || !d.raster || (r.hs && !d.hs.bytes) || (r.ds && !d.ds.bytes) || r.gs){++counters.dropped;return;}
    EffectPassBlendWitness(d.pass,d.blend,false,d.targets,d.depth);
    {   // I-0113 tranche 31 (effect-census.flag, gap 16): the 0x2D draws counted as EffectPassBlendWitness counts them (before the
        // drops below), by bank, programme and t0 (ffx_draw_observe::EffectCensusCount); each programme of another bank than 8
        // written once beside the journal (ffx-ps-census-<checksum>.dxbc, 32 at most).
        static const bool census=ModuleFlag(L"effect-census.flag");
        if(census && d.pass==0x2D){uint64_t checksum{};if(d.ps.bytes->size()>=12)std::memcpy(&checksum,d.ps.bytes->data()+4,8);
            const auto* token=effect_draw::AsToken(r.ps_resources[0]);
            const char* t0=!r.ps_resources[0]?"none":token && token->magic==effect_draw::copy_magic?"copy":"tex";
            ffx_draw_observe::EffectCensusCount("native",d.pass,d.blend,d.blend==8,checksum,t0);
            if(d.blend!=8){static std::mutex lock;static std::set<uint64_t> written;bool fresh{};{const std::lock_guard hold(lock);fresh=written.size()<32 && written.insert(checksum).second;}
                wchar_t name[64]{};swprintf_s(name,L"ffx-ps-census-%016llX.dxbc",static_cast<unsigned long long>(_byteswap_uint64(checksum)));
                if(FILE* file{};fresh && !_wfopen_s(&file,JournalSibling(name).c_str(),L"wb") && file){fwrite(d.ps.bytes->data(),1,d.ps.bytes->size(),file);fclose(file);}}}}
    uint32_t vertex_at{},index_at{};
    if(!Place(r.vertex_buffer,r.vertex_offset,vertex_at) || (r.indexed && !Place(r.index_buffer,r.index_offset,index_at))){++counters.foreign_buffers;++counters.dropped;return;}
    if(!d.sampler){++counters.other_samplers;++counters.dropped;return;}
    if(r.constants_invalid){++counters.dropped;return;}   // review: a partial or unreadable update (a never-updated block is zero)
    for(uint32_t c=0;c<EffectConstantSlots;++c)d.constants[c]=Range(r.constants[c]);
    for(uint32_t t=0;t<5;++t)if(!Resolve(r.ps_resources[t],d.textures[t])){++counters.dropped;return;}
    // I-0140 tranche 10: its target (no view or an engine view bound: nothing native to draw into), viewport and scissor; a draw
    // sampling the target it draws into is refused (D3D11 unbinds such a view; the original never binds one so, 271B40 copies first).
    if(r.target==effect_draw::target_unbound || r.target==effect_draw::target_foreign || r.target>=effect_draw::effect_targets ||
       (r.target && std::any_of(d.textures.begin(),d.textures.end(),[&](const NativeEffectTexture& t){return t.kind==NativeEffectTextureKind::Target && t.object==r.target;}))){
        ++counters.target_drops;++counters.dropped;return;}
    d.target=r.target;d.viewport_set=r.viewport_set;d.viewport=r.viewport;d.scissor_set=r.scissor_set;d.scissor=r.scissor;
    {   // Witness (ffx-record.flag): the classic draws that bind the default texture, by pass, topology, slot and pixel programme size.
        static const bool witness=ModuleFlag(L"ffx-record.flag");static std::map<std::string,uint64_t> uses;static uint64_t frames_seen{},last_frame{};
        if(witness){for(uint32_t t=0;t<5;++t)if(const auto* token=effect_draw::AsToken(r.ps_resources[t]);token && token->magic==effect_draw::default_magic){
                char key[64]{};sprintf_s(key,"%X/%u/t%u/ps%zu",unsigned(r.pass),unsigned(r.topology),t,d.ps.bytes?d.ps.bytes->size():0);++uses[key];}
            if(last_frame!=counters.frames){last_frame=counters.frames;if(++frames_seen%3000==1500){std::string row="{\"event\":\"native_ffx_default_uses\",\"frames\":"+std::to_string(frames_seen)+",\"uses\":{";
                bool first=true;for(const auto& [k,v]:uses){row+=(first?"\"":",\"")+k+"\":"+std::to_string(v);first=false;}row+="}}\n";Log(row.c_str());}}}}
    d.vertex_offset=vertex_at;d.stride=r.stride;d.index_offset=index_at;d.index_format=static_cast<DXGI_FORMAT>(r.index_format);
    d.indexed=r.indexed;d.count=r.count;d.start=r.start;d.base=r.base;
    {   // I-0105 (Lightning Spear's tracer, GX 11 patches drawn but unseen), ffx-record.flag: the first 12 such draws with a hull
        // stage: the bank states, the textures (kind, FFX id), the layout, the first 16 indices and the vertices they name (up to 4).
        static const bool witness=ModuleFlag(L"ffx-record.flag");static int tracer_rows{};
        if(witness && r.topology==11 && r.hs && tracer_rows<12){++tracer_rows;const auto& arena=effect_draw::Arena();const size_t used=std::min(effect_draw::ArenaUsed(),arena.size());
            std::string row;char cell[200]{};
            sprintf_s(cell,"{\"event\":\"native_ffx_tracer_record\",\"row\":%d,\"count\":%u,\"start\":%u,\"base\":%d,\"stride\":%u,\"index_format\":%u,\"blend\":\"%X\",\"depth\":\"%X\",\"raster\":\"%X\",\"targets\":%u,\"t\":[",
                tracer_rows,d.count,d.start,d.base,d.stride,unsigned(d.index_format),d.blend,d.depth,d.raster,d.targets);row=cell;
            for(uint32_t t=0;t<5;++t){const auto* token=effect_draw::AsToken(r.ps_resources[t]);int32_t id=-1;
                if(token && token->magic==effect_draw::texture_magic){const std::lock_guard hold(ffx_texture_id_lock);const auto f=ffx_texture_ids.find(token->payload);if(f!=ffx_texture_ids.end())id=f->second;}
                sprintf_s(cell,"%s[%d,%d]",t?",":"",int(d.textures[t].kind),id);row+=cell;}
            row+="],\"layout\":[";for(size_t i=0;i<d.layout->elements.size();++i){const auto& e=d.layout->elements[i];
                sprintf_s(cell,"%s\"%s%u/%d/%u\"",i?",":"",e.semantic.c_str(),e.index,int(e.format),e.offset);row+=cell;}
            row+="],\"indices\":[";std::vector<uint32_t> named;
            for(uint32_t i=0;i<16 && i<d.count;++i){uint16_t v{};const size_t at=size_t(index_at)+2u*(size_t(d.start)+i);if(d.index_format==DXGI_FORMAT_R16_UINT && at+2<=used)std::memcpy(&v,arena.data()+at,2);
                sprintf_s(cell,"%s%u",i?",":"",v);row+=cell;if(named.size()<4 && std::find(named.begin(),named.end(),v)==named.end())named.push_back(v);}
            row+="],\"vertices\":[";
            for(size_t k=0;k<named.size();++k){row+=k?",[":"[";const int64_t at=int64_t(vertex_at)+(int64_t(named[k])+d.base)*int64_t(d.stride);   // a negative base reads nothing
                for(uint32_t w=0;w<d.stride/4 && w<24;++w){float f{};const bool inside=at>=0 && uint64_t(at)+4u*w+4<=used;if(inside)std::memcpy(&f,arena.data()+at+4u*w,4);
                    if(inside && std::isfinite(f))sprintf_s(cell,"%s%.4g",w?",":"",f);else sprintf_s(cell,"%snull",w?",":"");row+=cell;}row+="]";}   // packed words may be NaN
            row+="]}\n";Log(row.c_str());}}
    {   // Witness (ffx-record.flag): every classic draw of frames 1800 and 2400, one row each: pass, topology, counts, programmes,
        // bank states, the textures (kind; an FFX texture by id*4+kind), the first vertex's first 16 bytes and the PS b0/VS b0 heads.
        // I-0113 gap 12 (Flame Fan's fan): F9 (game window in front) dumps the next effect frame too, at most 4 a session.
        static const bool witness=ModuleFlag(L"ffx-record.flag");static uint64_t key_frame{},key_seen{};static bool f9_down{};static int f9_taken{};
        if(witness && key_seen!=counters.frames){key_seen=counters.frames;DWORD owner{};GetWindowThreadProcessId(GetForegroundWindow(),&owner);
            const bool down=owner==GetCurrentProcessId() && (GetAsyncKeyState(VK_F9)&0x8000);if(down && !f9_down && f9_taken<4){++f9_taken;key_frame=counters.frames+1;}f9_down=down;}
        if(witness && (counters.frames==1800 || counters.frames==2400 || counters.frames==key_frame)){
            const auto& arena=effect_draw::Arena();const size_t used=std::min(effect_draw::ArenaUsed(),arena.size());
            std::string row="{\"event\":\"native_ffx_draw_dump\",\"frame\":"+std::to_string(counters.frames);char cell[260]{};
            sprintf_s(cell,",\"pass\":\"%X\",\"topology\":%u,\"count\":%u,\"indexed\":%d,\"stride\":%u,\"vs\":\"%llX\",\"ps\":\"%llX\",\"ps_bytes\":%zu,\"blend\":\"%X\",\"depth\":\"%X\",\"raster\":\"%X\",\"targets\":%u,\"t\":[",
                unsigned(r.pass),unsigned(d.topology),d.count,int(d.indexed),d.stride,static_cast<unsigned long long>(d.vs.key),static_cast<unsigned long long>(d.ps.key),d.ps.bytes?d.ps.bytes->size():0,d.blend,d.depth,d.raster,d.targets);row+=cell;
            for(uint32_t t=0;t<5;++t){const auto* token=effect_draw::AsToken(r.ps_resources[t]);int32_t id=-1;
                if(token && token->magic==effect_draw::texture_magic){const std::lock_guard hold(ffx_texture_id_lock);const auto f=ffx_texture_ids.find(token->payload);if(f!=ffx_texture_ids.end())id=f->second;}
                sprintf_s(cell,"%s[%d,%d]",t?",":"",int(d.textures[t].kind),id);row+=cell;}
            row+="],\"v0\":[";float v[4]{};if(vertex_at+16<=used)std::memcpy(v,arena.data()+vertex_at,16);
            sprintf_s(cell,"%.4g,%.4g,%.4g,%.4g]",v[0],v[1],v[2],v[3]);row+=cell;
            const size_t limit=d.ps.bytes && d.ps.bytes->size()==6752?96:12;   // the point sprites (the m37 snow): whole blocks
            const auto head=[&](const char* name,uint32_t slot){row+=",\"";row+=name;row+="\":[";const auto [data,size]=Bytes(d.constants[slot]);
                for(size_t i=0;i*4<size && i<limit;++i){float x{};std::memcpy(&x,data+i*4,4);sprintf_s(cell,"%s%.4g",i?",":"",x);row+=cell;}row+="]";};
            head("vs_b0",EffectVsB0);head("ds_b0",EffectDsB0);head("ps_b0",EffectPsB0);head("ps_b1",EffectPsB1);
            if(limit==12){sprintf_s(cell,",\"hs\":\"%llX\",\"ds\":\"%llX\"",static_cast<unsigned long long>(d.hs.key),static_cast<unsigned long long>(d.ds.key));row+=cell;}
            if(counters.frames==key_frame){static std::set<uint64_t> written;   // the F9 frame's programmes beside the journal (ffx-<stage>-<key>.dxbc), for fxc /dumpbin
                for(const auto& [program,stage]:{std::pair{&d.vs,L"vs"},{&d.hs,L"hs"},{&d.ds,L"ds"},{&d.ps,L"ps"}})
                    if(program->bytes && !program->bytes->empty() && written.insert(program->key).second){wchar_t name[64]{};swprintf_s(name,L"ffx-%s-%llX.dxbc",stage,static_cast<unsigned long long>(program->key));
                        if(FILE* file{};!_wfopen_s(&file,JournalSibling(name).c_str(),L"wb") && file){fwrite(program->bytes->data(),1,program->bytes->size(),file);fclose(file);}}}
            if(limit>12 && d.layout){row+=",\"layout\":[";for(size_t i=0;i<d.layout->elements.size();++i){const auto& e=d.layout->elements[i];
                sprintf_s(cell,"%s\"%s%u/%d/%u\"",i?",":"",e.semantic.c_str(),e.index,int(e.format),e.offset);row+=cell;}row+="]";
                sprintf_s(cell,",\"hs\":\"%llX\",\"ds\":\"%llX\",\"vb\":\"",static_cast<unsigned long long>(d.hs.key),static_cast<unsigned long long>(d.ds.key));row+=cell;
                for(uint32_t i=0;i<36 && vertex_at+i<used;++i){sprintf_s(cell,"%02X",arena[vertex_at+i]);row+=cell;}row+="\"";}
            row+="}\n";Log(row.c_str());}}
    frame_->draws.push_back(std::move(d));++counters.draws;}
}
// I-0113 gap 14 (Lightning Arrow's bow s08290, one FLVER Dynamic mesh on 12 bones, drawn too tall with its string in segments): a
// model particle's palettes as the original makes them. 636F0 hands its bone array A to 63CB0 with no pose object (the second word
// of its bone block null), so 64340 takes 64B30 (140064B30) for each mesh drawn, from the model M, its mesh record R ([M+0xA8] +
// 0x48 x the mesh ordinal: R+4 Dynamic (INFERRED from s08290: its one mesh Dynamic, skinned by BLENDINDICES, its vertices in the
// model's bind space), R+0x16 the default bone, R+0x18 the list's size, [R+0x20] the list) and the bind rows G ([M+0x1D0], read
// only when a mesh is Dynamic): ds3content::ProduceEffectModelPalettes holds the rule. Before, every mesh took A as full rows.
// False, with `skip` named, when a record, a list or G is unreadable or an index is past A (I-0101: the particle skipped this frame).
// A non-gated Dynamic mesh whose list is empty (or starts with a negative entry) gets a palette of no rows, which binds nothing:
// that mesh is refused (native_selected_palette_unavailable), where the original uploads no row and draws on the b5 it had; a list
// ended early by a negative entry gives fewer rows than its +0x18 (the original uploads +0x18, the rest unwritten). The non-gated
// branch is READ only: every model particle of the tranche-32 runs has +0x1E0 set.
bool ComposeNativeEffectModelPoses(const effect_draw::ModelSubmission& m,bool auxiliary_present,NativeEffectModel& item,const char*& skip) {
    const auto snapshot=[&](const std::vector<PoseRow>& rows){
        ds3content::ModelPoseOwner palettes(static_cast<uint32_t>(rows.size()));std::vector<PoseRow> gates;
        return std::shared_ptr<const NativeModelPoseSnapshot>(ComposeNativeModelPose(m.model,rows,false,true,auxiliary_present,false,0,palettes,gates,skip));};
    item.pose=snapshot(m.rows);if(!item.pose)return false;
    BYTE full{};int32_t count{};uintptr_t meshes{};
    if(!Read(m.model+0x1E0,&full,1) || !Read(m.model+0xA0,&count,4) || count<0 || count>4096 || (count && (!Read(m.model+0xA8,&meshes,8) || !meshes))){
        skip="effect_model_meshes_unreadable";return false;}
    std::vector<ds3content::EffectModelMesh> records(static_cast<size_t>(count));bool dynamic=false;
    for(int32_t i=0;i<count;++i){std::array<uint8_t,0x28> r{};auto& mesh=records[size_t(i)];
        if(!Read(meshes+0x48ull*uint64_t(i),r.data(),r.size())){skip="effect_model_meshes_unreadable";return false;}
        uint16_t listed{};uintptr_t list{};std::memcpy(&mesh.default_bone,r.data()+0x16,2);std::memcpy(&listed,r.data()+0x18,2);std::memcpy(&list,r.data()+0x20,8);
        mesh.dynamic=r[4]!=0;dynamic|=mesh.dynamic;
        if(!full && listed){mesh.bones.resize(listed);if(!list || !Read(list,mesh.bones.data(),mesh.bones.size()*4)){skip="effect_model_bone_list_unreadable";return false;}}}
    if(!dynamic && full)return true;   // every mesh takes A: the pose as given
    std::vector<PoseRow> bind;uintptr_t at{};
    if(dynamic && !m.rows.empty()){bind.resize(m.rows.size());
        if(!Read(m.model+0x1D0,&at,8) || !at || !Read(at,bind.data(),bind.size()*sizeof(PoseRow))){skip="effect_model_bind_rows_unreadable";return false;}}
    std::vector<ds3content::EffectMeshPalette> palettes;std::vector<PoseRow> composed;
    if(!ds3content::ProduceEffectModelPalettes(full!=0,m.rows,bind,records,palettes,composed,skip))return false;
    std::shared_ptr<const NativeModelPoseSnapshot> shared;   // the A x G full rows, one version for every Dynamic mesh
    item.mesh_poses.resize(palettes.size());
    for(size_t i=0;i<palettes.size();++i)switch(palettes[i].kind){
        case ds3content::EffectMeshPaletteKind::Given:break;
        case ds3content::EffectMeshPaletteKind::Composed:if(!shared && !(shared=snapshot(composed)))return false;item.mesh_poses[i]=shared;break;
        case ds3content::EffectMeshPaletteKind::Own:if(!(item.mesh_poses[i]=snapshot(palettes[i].rows)))return false;break;}
    return true;
}
// N5a per frame (the main view's collect, engine thread): the scene-control draw of pass 0x45 into the native pass context,
// the recorded commands counted by their engine pass (the data block's +0x08) and logged every 600th frame.
void RecordNativeEffects(NativeSelectedScene& scene,uintptr_t view) {
    using namespace native_ffx;
    static const bool enabled=native_ffx_draw,witness=ModuleFlag(L"ffx-record.flag");   // ffx-record.flag: the witnesses (rows below)
    if(!enabled)return;
    static LONG64 skipped[4]{};const auto skip=[&](int why){if(InterlockedIncrement64(&skipped[why])==1){char row[96]{};
        sprintf_s(row,"{\"event\":\"native_ffx_record_skipped\",\"reason\":%d}\n",why);Log(row);}};
    if(!native_effect_published.load(std::memory_order_acquire) || !native_effect_system.scene_control){skip(0);return;}
    {   // Witness, once (FFX-DRAW N6): the shader set's programmes. The container's VS lookup (vt+0x38, repository 4792D58) for
        // GXFfxtessellateTexture: the object, its vt+0x10 (what 291520 stores as the VS), and which of the object's words or
        // offsets the native shader registry knows; then the set's first entries (+0x118 on, 0x38 each: layout, VS, HS, DS, GS,
        // PS, +0x30), each word classified.
        static bool probed{};if(witness && !probed){probed=true;{char t[96]{};sprintf_s(t,"{\"event\":\"native_ffx_record_thread\",\"thread\":%lu}\n",GetCurrentThreadId());Log(t);}
            const auto classify=[](uintptr_t p)->std::string{if(!p)return "0";{const std::lock_guard hold(shader_lock);if(shaders.count(p))return "shader";}
                uint32_t kind{};std::array<uint32_t,8> d{};if(effect_system::Describe(p,kind,d))return "effect"+std::to_string(kind);
                uintptr_t vt{};if(Read(p,&vt,8) && vt>image && vt<image+0x5000000){char c[24]{};sprintf_s(c,"vt%llX",static_cast<unsigned long long>(vt-image));return c;}return "other";};
            uintptr_t container{},table{},lookup{};Read(native_effect_system.manager+0x38,&container,8);if(container)Read(container,&table,8);if(table)Read(table+0x38,&lookup,8);
            std::string row="{\"event\":\"native_ffx_shader_probe\",\"container\":\""+classify(container)+"\"";
            if(lookup){const auto object=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t,uint64_t,const wchar_t*)>(lookup)(container,0,L"GXFfxtessellateTexture");
                row+=",\"object\":\""+classify(object)+"\"";
                if(object){uintptr_t vt{},getter{};Read(object,&vt,8);if(vt)Read(vt+0x10,&getter,8);
                    const auto got=getter?reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(getter)(object):0;
                    char c[96]{};sprintf_s(c,",\"getter\":\"%llX\",\"got\":\"",static_cast<unsigned long long>(getter?getter-image:0));row+=c+classify(got)+"\",\"offsets\":[";
                    bool first=true;for(uint32_t off=0;off<0x100;off+=8){const auto at=classify(object+off);uintptr_t word{};Read(object+off,&word,8);const auto in=classify(word);
                        if(at=="shader" || in=="shader"){char cell[64]{};sprintf_s(cell,"%s\"%X:%s/%s\"",first?"":",",off,at.c_str(),in.c_str());row+=cell;first=false;}}
                    row+="]";}}
            uintptr_t set{};Read(native_effect_system.scene_control+0xE0,&set,8);row+=",\"entries\":[";
            for(uint32_t i=5;set && i<13;++i){row+=(i>5?",[":"[");for(uint32_t w=0;w<7;++w){uintptr_t word{};Read(set+i*0x38+w*8,&word,8);row+=(w?",\"":"\"")+classify(word)+"\"";}row+="]";}
            Log((row+"]}\n").c_str());}}
    uintptr_t context{},camera{};
    {const std::lock_guard<std::recursive_mutex> hold(scene_view_cpu_lock);
        const auto found=scene_view_cpu_owners.find(view);if(found!=scene_view_cpu_owners.end())context=found->second.context;}
    if(!context){skip(1);return;}
    // The pass context's camera +0x418: the GX camera record of the main view (natively CED670's output, the view context's
    // +0x210 being empty: Native1929); +0x30 the position, +0x40 the target, +0x54 near, +0x58 far (Reference1925 at the title).
    camera=native_scene_camera_object.load(std::memory_order_acquire);if(!camera){skip(2);return;}
    std::array<float,16> pose{};if(!Read(camera,pose.data(),64))return;
    if(ffx_draw_observe::camera_rows.load()>0 && ffx_draw_observe::camera_rows.fetch_sub(1)>0){std::array<float,12> cam{};Read(camera+0x30,cam.data(),48);for(auto& x:cam)if(!std::isfinite(x))x=0;   // I-0141, F9
        char text[400]{};sprintf_s(text,"{\"event\":\"native_ffx_camera\",\"camera\":\"%llX\",\"camera_30\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g]}\n",static_cast<unsigned long long>(camera),
            cam[0],cam[1],cam[2],cam[3],cam[4],cam[5],cam[6],cam[7],cam[8],cam[9],cam[10],cam[11]);Log(text);}
    // The depth row: the camera's forward row (+0x20 of its world matrix, +0x30 the position), w = -dot(row, position)
    // (Reference1925: the title camera at (0, 4, -8) gives (0, -0.447, 0.894, 8.944)). INFERRED layout, checked by the witness.
    // I-0113 tranche 15: the forward rows' keys take the same row (SortNativeForwardRows).
    float row[4]{};if(!ds3native::sort_key::DepthRow(pose.data(),row))return;
    const auto volume=reinterpret_cast<uintptr_t>(view_volume.data());
    if(!view_volume_built){reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x6E780)(volume);view_volume_built=true;}
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0x6E9E0)(volume,camera);
    if(ffx_draw_observe::frustum_rows.load()>0 && ffx_draw_observe::frustum_rows.fetch_sub(1)>0)   // I-0141, F9: the frustum 6E9E0 made and the camera
        Log(("{\"event\":\"native_ffx_frustum\",\"frustum\":\""+ffx_draw_observe::HexWords(volume,0x300)+"\",\"camera\":\""+ffx_draw_observe::HexWords(camera,0x60)+"\"}\n").c_str());
    auto* c=pass_context.data();std::memset(c,0,pass_context.size());commands.clear();blocks_used=0;
    const auto put=[&](size_t at,const void* v,size_t n){std::memcpy(c+at,v,n);};
    const int32_t pass=0x45,one=1;const uintptr_t frustum=volume,sort=reinterpret_cast<uintptr_t>(sort_block.data()),
        pipeline=reinterpret_cast<uintptr_t>(pipeline_block.data());
    put(0x124,&pass,4);put(0x410,&frustum,8);put(0x418,&camera,8);put(0x250,row,16);put(0x4B0,&sort,8);put(0x4B8,&one,4);put(0x4BC,&one,4);
    // I-0113 tranche 33 (gap 16): +0x110, the float4 264230 multiplies into the three SH rows' .w words (+0x110, +0x114, +0x118, READ
    // in its disassembly at 140265BF3): the world render task's (f8 f7, f9 f7, f10 f7, 1) of the view's two Dominant Light records
    // (140204C30, the rows cbPerFrame takes: ProducePassDominantScale), the base pass context's (1, 1, 1, 1) without them. Zero
    // before: every native effect draw through 264230 lost the SH rows' constant term (i113-t33-orbw: 0 natively, 1 in the original).
    // Tranche 34: the scene's one value (pass_row_scale, made at the collect), which the meshes, decals and model particles read too.
    {const float scale[4]{scene.pass_row_scale[0],scene.pass_row_scale[1],scene.pass_row_scale[2],1.f};put(0x110,scale,16);}
    for(const int p:{0x1F,0x29,0x2D,0x31,0x34})c[0x426+p]=1;
    put(0x4F0,&pipeline,8);put(0x58C,&one,4);
    // N6: the frame copy +0x4C0 and depth +0x4C8 (copied into each command's data +0x20/+0x28, read by 299E40 through their +0x48):
    // native texture objects whose views are the copy and depth tokens (effect_draw), for the programmes that sample them.
    {const uintptr_t copy=effect_draw::FrameCopyTexture(),depth=effect_draw::DepthTexture();put(0x4C0,&copy,8);put(0x4C8,&depth,8);}
    const auto gate=DrawGateEnter(native_effect_system.scene_control,Context());   // I-0141 tranche 14: ffx_draw_gate
    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t)>(image+0x24E790)(native_effect_system.scene_control,Context());
    DrawGateLeave(gate,native_effect_system.scene_control,Context());
    if(witness)for(const auto& command:commands){int32_t engine_pass{};if(command.data)Read(command.data+8,&engine_pass,4);++recorded[engine_pass];}   // I-0120 D5: read only by the ffx-record.flag summary
    // N5b/N6, I-0104: the commands executed through the native effect context (effect_draw), the frame the renderer draws made as they run.
    {std::vector<effect_draw::Command> run;run.reserve(commands.size());
        for(const auto& recorded_command:commands)run.push_back({recorded_command.callback,recorded_command.entity,recorded_command.data,recorded_command.key,static_cast<int32_t>(recorded_command.pass)});
        static const bool gpu_particles=!ModuleFlag(L"no-ffx-gpu.flag");   // E07 on by default (D-0186), no-ffx-gpu.flag opts out
        static const bool model_particles=!ModuleFlag(L"no-ffx-models.flag");   // E03 on by default (I-0105), no-ffx-models.flag opts out
        static const bool trails=!ModuleFlag(L"no-ffx-trails.flag");   // 606 and 10012 on by default (I-0105), no-ffx-trails.flag opts out
        static const bool light_shafts=!ModuleFlag(L"no-ffx-lightshafts.flag");   // 10003 on by default (I-0140 tranche 10), no-ffx-lightshafts.flag opts out
        EffectFrameBuilder builder;   // I-0104: each statement made into the frame's draw at once
        // E03 (I-0105): each model particle's FLVER draw, with its palettes made from its bone rows as 636F0 receives them, mesh by
        // mesh as 64B30 makes them (I-0113 gap 14, ComposeNativeEffectModelPoses), each through a palette owner of its own; the rows
        // are then dropped (the poses hold them). The main collect admits them after its walk.
        std::vector<std::shared_ptr<const NativeEffectModel>> models;
        const auto model=[&](effect_draw::ModelSubmission&& m){
            uint32_t flags{};if(!Read(m.model+0x224,&flags,4)){NoteNativeSkip("effect_model_unreadable",m.model);return;}
            auto item=std::make_shared<NativeEffectModel>();
            if(witness){std::string name;for(const auto ch:NativeModelDataName(m.model))name.push_back(ch>=32 && ch<127 && ch!='"' && ch!='\\'?ch:'?');
                ffx_draw_observe::ModelRowsWitness("native",name,m.model,m.lod,m.rows.empty()?nullptr:reinterpret_cast<const float*>(m.rows.data()),m.rows.size());}
            const char* skip{};
            if(!ComposeNativeEffectModelPoses(m,(flags&0x4000)!=0,*item,skip)){NoteNativeSkip(skip,m.model);return;}
            m.rows=std::vector<PoseRow>{};item->submission=std::move(m);models.push_back(std::move(item));};
        effect_draw::Execute({image,&Read,&Log,gpu_particles,NativeRenderWidth(),NativeRenderHeight(),witness,model_particles,trails,light_shafts},Context(),native_effect_system.scene_control,std::move(run),   // I-0111: the world's extent
            [&](const effect_draw::EffectDrawRecord& r){builder.Statement(r);},model);
        {const std::array<float,2> jitter{scene.camera.jitter_x,scene.camera.jitter_y};   // I-0116: the scene camera's (main view, DLSS)
            const bool jittered=jitter[0]!=0 || jitter[1]!=0;VelocityMatrix vp{};   // unjittered, the effects' world-to-clip layout (under the jitter only)
            const bool projected=jittered && VelocityViewProjection(scene.camera,vp);
            scene.effects=builder.Finish(upscaler::JitterNdc(jitter,scene.camera.width,scene.camera.height),jittered,projected?&vp:nullptr);}   // N6: the renderer draws them (scene → frame input)
        scene.effect_models=std::move(models);
        if(witness && frames%600==599){const auto& fc=effect_frame_counters;char counts[420]{};
            sprintf_s(counts,",\"frame\":{\"frames\":%llu,\"draws\":%llu,\"dropped\":%llu,\"unresolved_textures\":%llu,\"default_textures\":%llu,\"foreign_buffers\":%llu,\"other_samplers\":%llu,\"copy_rects\":%llu,\"target_statements\":%llu,\"target_drops\":%llu}",
                static_cast<unsigned long long>(fc.frames),static_cast<unsigned long long>(fc.draws),static_cast<unsigned long long>(fc.dropped),
                static_cast<unsigned long long>(fc.unresolved_textures),static_cast<unsigned long long>(fc.default_textures),
                static_cast<unsigned long long>(fc.foreign_buffers),static_cast<unsigned long long>(fc.other_samplers),static_cast<unsigned long long>(fc.copy_rects),
                static_cast<unsigned long long>(fc.target_statements),static_cast<unsigned long long>(fc.target_drops));
            Log(("{\"event\":\"native_ffx_execute\",\"summary\":"+effect_draw::Summary()+counts+"}\n").c_str());}}
    if(witness && frames==1200)Log(("{\"event\":\"native_ffx_type_registry\",\"registry\":"+FfxTypeRegistry()+"}\n").c_str());
    if(++frames;witness && (frames==1 || frames%600==0)){std::string counts;for(const auto& [k,v]:recorded){char cell[40]{};sprintf_s(cell,"%s\"%X\":%llu",counts.empty()?"":",",k,static_cast<unsigned long long>(v));counts+=cell;}
        std::array<uint8_t,0x60> bytes{};Read(camera,bytes.data(),bytes.size());
        uintptr_t mgr_vt{},library_draw{};Read(native_effect_system.ffx_manager,&mgr_vt,8);if(mgr_vt)Read(mgr_vt+0x20,&library_draw,8);
        // The library draw 142155520: groups FUN_141AD1630([mgr+8]) chained by 140249680, entities by 140249690 and 141AC9BD0, each
        // offered through its vt+0x38 (which calls the pass slot 4); counted here without drawing.
        uint64_t groups{},listed{};{uintptr_t mgr8{};Read(native_effect_system.ffx_manager+8,&mgr8,8);
            for(auto g=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x1AD1630)(mgr8);g && groups<100000;g=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x249680)(g)){++groups;
                for(auto e=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x249690)(g);e && listed<1000000;e=reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x1AC9BD0)(e)){++listed;uintptr_t vt{},slot{};if(Read(e,&vt,8) && vt && Read(vt+0x38,&slot,8) && slot)++entity_slots[{vt-image,slot-image}];
                    if(slot==image+0x1AD6440){uint32_t b0{};Read(e+0xB0,&b0,4);if(b0&1){++node_counts[0];
                        // 1ADB0C0 over the entity's node chain (e+0xD0, then +0xB0 links): 1ADDE80 needs +0x70, +0x58 (renderer), +0x68 (resource).
                        for(uintptr_t node=e+0xD0,guard=0;node && guard<64;++guard){uint32_t n70{};uintptr_t n58{},n68{};Read(node+0x70,&n70,4);Read(node+0x58,&n58,8);Read(node+0x68,&n68,8);
                            FfxNodeKinds(node,node_kinds);
                            ++node_counts[1];if(n70)++node_counts[2];if(n58)++node_counts[3];if(n68)++node_counts[4];if(n70 && n58 && n68)++node_counts[5];
                            uintptr_t next{};if(!Read(node+0xB0,&next,8) || next==node)break;node=next;}}}}}}
        {std::string slots;for(const auto& [k,v]:entity_slots){char cell[64]{};sprintf_s(cell,"%s\"%llX/%llX\":%llu",slots.empty()?"":",",static_cast<unsigned long long>(k.first),static_cast<unsigned long long>(k.second),static_cast<unsigned long long>(v));slots+=cell;}
            char nodes[200]{};sprintf_s(nodes,",\"active\":%llu,\"nodes\":[%llu,%llu,%llu,%llu,%llu]",static_cast<unsigned long long>(node_counts[0]),static_cast<unsigned long long>(node_counts[1]),
                static_cast<unsigned long long>(node_counts[2]),static_cast<unsigned long long>(node_counts[3]),static_cast<unsigned long long>(node_counts[4]),static_cast<unsigned long long>(node_counts[5]));node_counts.fill(0);
            entity_slots.clear();Log(("{\"event\":\"native_ffx_entity_slots\",\"slots\":{"+slots+"}"+nodes+"}\n").c_str());
            Log(("{\"event\":\"native_ffx_node_kinds\",\"kinds\":"+FfxKinds(node_kinds)+"}\n").c_str());node_kinds.clear();}
        uint8_t sc_flags[4]{};Read(native_effect_system.scene_control+0x20,&sc_flags[0],1);Read(native_effect_system.scene_control+0x5BE,&sc_flags[1],1);Read(native_effect_system.scene_control+0x590,&sc_flags[2],1);
        { char extra[320]{};sprintf_s(extra,"{\"event\":\"native_ffx_record_state\",\"groups\":%llu,\"listed\":%llu,\"entities_offered\":%llu,\"library_draw\":\"%llX\",\"sc_20\":%u,\"sc_5be\":%u,\"sc_590\":%u}\n",static_cast<unsigned long long>(groups),static_cast<unsigned long long>(listed),static_cast<unsigned long long>(entity_calls),static_cast<unsigned long long>(library_draw?library_draw-image:0),sc_flags[0],sc_flags[1],sc_flags[2]);Log(extra);}
        char head[300]{};sprintf_s(head,"{\"event\":\"native_ffx_record\",\"frames\":%llu,\"this_frame\":%zu,\"row\":[%g,%g,%g,%g],",static_cast<unsigned long long>(frames),commands.size(),row[0],row[1],row[2],row[3]);
        Log((std::string(head)+"\"recorded\":{"+counts+"},\"camera\":\""+HexBytes(bytes.data(),bytes.size())+"\"}\n").c_str());}
}
// W4 FW-3: SceneCtrl slot 8 FUN_14024D000 (vt+0x40 of the scene control, called by E6E750 once +0x58 is published): the
// native scene control's update is the transcription effect_system::Update; any other receiver reaches the original.
using SceneCtrlUpdate=void(__fastcall*)(uintptr_t,float);
SceneCtrlUpdate scene_ctrl_update_original{};
void __fastcall NativeSceneCtrlUpdate(uintptr_t scene_control,float dt) noexcept {
    if(!scene_control || scene_control!=native_effect_system.scene_control){scene_ctrl_update_original(scene_control,dt);return;}
    static const bool run_cpu_items=native_ffx_draw;   // FFX-DRAW: the particle simulation items (the draws need the live counts) and the message items
    // I-0092: the node refills and the particle items on the admission pool (free in DS3's SFX step; serial-effects.flag A/B: on
    // this thread); the item kinds counted for the update whose row is logged.
    static WorkPool* const pool=ModuleFlag(L"serial-effects.flag")?nullptr:&AdmissionPool();
    const auto next=effect_system::Updates().updates+1;
    const effect_system::Engine engine{image,&Read,&Log,run_cpu_items,false,pool,next<=3 || next%600==0};
    effect_system::Update(engine,scene_control,dt);
    const auto stats=effect_system::Updates();
    if(stats.updates<=3 || stats.updates%600==0){
        const auto objects=effect_system::Buffers();
        char row[640]{};sprintf_s(row,"{\"event\":\"native_effect_update\",\"updates\":%llu,\"dt\":%g,\"nodes\":%llu,\"node_refills\":%llu,"
            "\"proceed_items\":%llu,\"update_items\":%llu,\"cpu_items_run\":%llu,\"native_objects\":%llu,\"native_buffer_bytes\":%llu,\"released\":%llu,\"thread\":%lu,\"kinds\":",
            stats.updates,dt,stats.nodes,stats.node_refills,stats.proceed_items,stats.update_items,stats.cpu_items_run,objects.buffers,objects.bytes,objects.released,GetCurrentThreadId());
        Log((std::string(row)+effect_system::ItemKinds()+"}\n").c_str());}   // review: the kinds are unbounded, never in a fixed row
}
void __fastcall NativeSfxStartup(void* step) noexcept {
    // R13 cR3 review: Close() can throw (the render worker's deadline or kept fault); this hook is noexcept, so the module's fault
    // exit is taken here instead of std::terminate.
    if(control->stop){try{Close();}catch(const std::exception& e){NativeFailure(e.what());}return;}
    if(!opened || !native_resource_consumers || GetCurrentThreadId()!=owner_thread)Refuse("native_sfx_phase_without_resources");
    uintptr_t repository{};BYTE state{};
    const auto receiver=reinterpret_cast<uintptr_t>(step);
    if(!Read(receiver+0x128,&repository,8) || !repository || !Read(repository+0x70,&state,1))Refuse("native_sfx_cpu_repository_unavailable");
    if(state!=4)return;
    if(!sfx_cpu_types_initialized){uintptr_t heap{},pools[2]{};
        if(!Read(image+0x479C270,&heap,8) || !heap ||
           !Read(image+0x4932768,pools,sizeof(pools)) || pools[0] || pools[1])
            Refuse("native_sfx_cpu_pools_not_pristine");
        // Exact CPU parser/type registration from24B260, with its actual
        // content heap in both arguments. No mixed SFX constructor follows.
        if(!reinterpret_cast<BYTE(__fastcall*)(uintptr_t,uintptr_t)>(image+0x1A97EE0)(heap,heap))
            Refuse("native_sfx_cpu_type_initialization_failed");
        sfx_cpu_types_initialized=true;
        Log("{\"event\":\"native_sfx_cpu_types_initialized\",\"genuine_memory_pools\":true,\"original_gpu_manager\":false,\"normal_pool_retirement_qualified\":false}\n");
    }
    // SFX rendering stays omitted: the CPU archive finished loading normally and
    // the engine's SFX manager is published; its GX effect system is built and published below (W4) unless no-native-effects.flag.
    if(!native_fx_world)try{PublishNativeFxWorld();}catch(const std::exception&){Refuse("native_fx_world_construction");}
    if(!native_sfx_manager)try{PublishNativeSfxManager();}catch(const std::exception&){Refuse("native_sfx_manager_construction");}
    if(!native_effect_system.scene_control && native_effects_enabled){
        const effect_system::Engine engine{image,&Read,&Log,false,native_ffx_draw};
        try{native_effect_system=effect_system::Build(engine,native_sfx_manager);}
        catch(const std::exception& e){char row[256]{};sprintf_s(row,"{\"event\":\"native_effect_system_refused\",\"step\":\"%.180s\"}\n",e.what());Log(row);
            Refuse("native_effect_system_construction");}
        if(!effect_system::Check(engine,native_effect_system))Refuse("native_effect_system_invariants");
        // W4 FW-3: publish (E6C9D0 stores +0x58 right after 24B260; here after the native construction and its check).
        {   // FW-9: published by default once built
            *reinterpret_cast<uintptr_t*>(native_sfx_manager+0x58)=native_effect_system.scene_control;
            native_effect_system.published=true;
            native_effect_published.store(true,std::memory_order_release);
            Log("{\"event\":\"native_effect_system_published\",\"sfx_manager_58\":true,\"door_7cb820\":true}\n");
        }
    }
    Log("{\"event\":\"native_optional_visual_deferred\",\"family\":\"sfx\",\"cpu_archive_state\":4,\"legacy_gpu_created\":false,\"sfx_manager_published\":true}\n");
    reinterpret_cast<Phase>(image+0xD5B250)(step);*reinterpret_cast<BYTE*>(receiver+0x58)=1;
    Observe(step,"native_sfx_deferred","leave");
}
uintptr_t __fastcall ConstructNativeSfxCpuRecord(uintptr_t receiver) noexcept {
    try {
        const auto result=sfx_cpu_construct_original(receiver);uintptr_t table{};uint32_t identifier{};
        if(result!=receiver || !Read(receiver,&table,8) || table!=image+0x297CA00 ||
           !Read(receiver+0x60,&identifier,4) || identifier!=UINT32_MAX)
            Refuse("native_sfx_cpu_constructor_contract");
        const std::lock_guard<std::mutex> lock(sfx_asset_lock);
        if(!sfx_asset_records.emplace(receiver,nullptr).second)Refuse("native_sfx_cpu_duplicate_record");
        return result;
    }catch(...){Refuse("native_sfx_cpu_construction_failed");}
}
void __fastcall RetireNativeSfxCpuRecord(uintptr_t receiver) noexcept {
    try {
        uintptr_t table{};uint32_t identifier{};
        if(!Read(receiver,&table,8) || table!=image+0x297CA00 || !Read(receiver+0x60,&identifier,4))
            Refuse("native_sfx_cpu_retirement_contract");
        std::shared_ptr<const NativeSfxAsset> retained;
        {const std::lock_guard<std::mutex> lock(sfx_asset_lock);
            const auto found=sfx_asset_records.find(receiver);
            if(found==sfx_asset_records.end())Refuse("native_sfx_cpu_foreign_retirement");
            retained=found->second;
            if(retained){
                if(retained->identifier!=identifier)Refuse("native_sfx_cpu_retirement_identity");
                const auto active=sfx_assets.find(identifier);
                if(active!=sfx_assets.end() && active->second==retained)sfx_assets.erase(active);
            }else if(identifier!=UINT32_MAX && !native_effect_published.load(std::memory_order_acquire))Refuse("native_sfx_cpu_unowned_identifier");
            sfx_asset_records.erase(found);
        }
        // Source625/626: duplicate insertion destroys an unfilled record.
        // E0DB20 asserts the absent old SFX manager before E6E600's genuine
        // UINT_MAX no-op. Its remaining tail is pure233BBD0 CPU name/heap
        // retirement. Loaded native records withdraw their own parsed lease.
        // W4 FW-3: with the effect system published, E0DB20's own statement runs: E6E600 releases the identifier from the
        // effect system's FXR cache ([sc+0x28]+0x160; UINT_MAX is a no-op there).
        if(native_effect_published.load(std::memory_order_acquire) && !retained){uint32_t key=identifier;
            reinterpret_cast<void(__fastcall*)(uintptr_t,uint32_t*)>(image+0xE6E600)(native_sfx_manager,&key);}
        reinterpret_cast<void(__fastcall*)(uintptr_t)>(image+0x233BBD0)(receiver);
        char row[224]{};sprintf_s(row,"{\"event\":\"native_sfx_cpu_record_retired\",\"identifier\":%u,\"loaded_asset\":%s,\"actual_cpu_base_retirement\":true,\"original_sfx_manager\":false}\n",identifier,retained?"true":"false");Log(row);
    }catch(...){Refuse("native_sfx_cpu_retirement_failed");}
}
void __fastcall NativeSfxAssetLoad(uintptr_t receiver,uintptr_t data,uint64_t size) noexcept {
    try {
        if(size<0x68 || size>(64u<<20) || !data)throw std::runtime_error("native FXR asset size");
        // W4 FW-3: with the effect system published, E0DD10's own statement runs: E6E570 registers the FXR in the effect
        // system (24ECF0 into the scene control's FXR cache) and returns its identifier, stored at +0x60. The record stays
        // owned by the engine (no native parsed copy); its retirement releases it through E6E600.
        if(native_effect_published.load(std::memory_order_acquire)){
            uintptr_t name=receiver+0x10;uint64_t capacity{};
            if(!Read(receiver+0x28,&capacity,8) || (capacity>7 && !Read(receiver+0x10,&name,8)))throw std::runtime_error("native FXR asset name");
            uint32_t identifier=UINT32_MAX;
            reinterpret_cast<uint32_t*(__fastcall*)(uintptr_t,uint32_t*,uintptr_t,uintptr_t,uint64_t)>(image+0xE6E570)(native_sfx_manager,&identifier,name,data,size);
            *reinterpret_cast<uint32_t*>(receiver+0x60)=identifier;
            static std::atomic<uint64_t> registered{};
            if(const auto n=++registered;n<=8 || n%500==0){char row[192]{};sprintf_s(row,"{\"event\":\"native_sfx_fxr_registered\",\"count\":%llu,\"identifier\":%u,\"bytes\":%llu}\n",
                n,identifier,static_cast<unsigned long long>(size));Log(row);}
            return;
        }
        auto owned=std::make_shared<NativeSfxAsset>();owned->bytes.resize(static_cast<size_t>(size));
        if(!Read(data,owned->bytes.data(),owned->bytes.size()))throw std::runtime_error("native FXR asset bytes");
        if(std::memcmp(owned->bytes.data(),"FXR",4) || owned->bytes[8]!=1)
            throw std::runtime_error("native FXR serialized family");
        uint32_t version{};std::memcpy(&version,owned->bytes.data()+4,4);
        if((version>>16)>=5)throw std::runtime_error("native FXR serialized version");
        // Genuine21561A0 relocates these eleven CPU sections in its own copy.
        // Bound its table walks before allowing the original CPU parser to run.
        constexpr uint32_t strides[]{0x10,0x10,0x60,0x30,0x20,0x40,0x28,0x20,0x18,0x10,4};
        for(size_t section=0;section<std::size(strides);++section){uint32_t offset{},count{};
            std::memcpy(&offset,owned->bytes.data()+0x10+section*8,4);
            std::memcpy(&count,owned->bytes.data()+0x14+section*8,4);
            if(offset>size || uint64_t(count)*strides[section]>size-offset)
                throw std::runtime_error("native FXR CPU section range");}
        owned->name=ReadCpuString(receiver+0x10);
        owned->identifier=reinterpret_cast<uint32_t(__fastcall*)(const wchar_t*)>(image+0xE69660)(owned->name.c_str());
        if(owned->identifier==UINT32_MAX)throw std::runtime_error("native FXR authored identifier");
        uintptr_t allocator_service{},heap{},pool{};
        if(!Read(image+0x4932770,&allocator_service,8) || !allocator_service ||
           !Read(allocator_service+0x28,&heap,8) || !heap || !Read(image+0x4932768,&pool,8) || !pool)
            throw std::runtime_error("native FXR real CPU allocator unavailable");
        // Actual CPU copy/relocation/validation and paired parsed-buffer
        // retirement. No mixed SFX manager or original GPU work is invoked.
        const auto parsed=reinterpret_cast<uintptr_t(__fastcall*)(const void*,int)>(image+0x2155F90)(owned->bytes.data(),static_cast<int>(size));
        if(!parsed)throw std::runtime_error("native FXR CPU parser refused");
        owned->parsed=std::shared_ptr<void>(reinterpret_cast<void*>(parsed),[](void* value){
            if(native_process_terminating.load(std::memory_order_acquire))return;   // the engine heap may be gone
            reinterpret_cast<uintptr_t(__fastcall*)(uintptr_t)>(image+0x2B7B80)(reinterpret_cast<uintptr_t>(value));});
        {const std::lock_guard<std::mutex> lock(sfx_asset_lock);
            const auto record=sfx_asset_records.find(receiver);
            if(record==sfx_asset_records.end() || record->second)Refuse("native_sfx_cpu_load_foreign_record");
            owned->generation=++sfx_asset_generation;sfx_assets.insert_or_assign(owned->identifier,owned);
            record->second=owned;}
        *reinterpret_cast<uint32_t*>(receiver+0x60)=owned->identifier;
        char row[256]{};sprintf_s(row,"{\"event\":\"native_sfx_cpu_asset\",\"generation\":%llu,\"identifier\":%u,\"bytes\":%llu,\"actual_cpu_parser\":true,\"original_sfx_manager\":false,\"native_effect_rendering\":false}\n",
            owned->generation,owned->identifier,size);Log(row);
    }catch(const std::exception& error){char row[320]{};
        sprintf_s(row,"{\"event\":\"native_sfx_cpu_error\",\"reason\":\"%s\"}\n",error.what());Log(row);
        Refuse("native_sfx_cpu_admission_failed");}
}
}  // namespace
