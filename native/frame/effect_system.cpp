// W4 FW-2: see effect_system.h. Statement order and offsets follow the decompiled original
// (decompiled/DS3-1152-F-00E6C9D0, -0024B260, -0024C530); the GX reachability of every function called as is was checked
// statically (evidence/I-0068/queries/ffx-w4/reach/gx-reach.txt: none reaches the display object, but 6E780, whose 4B5C0
// tests it for null first).
#include "effect_system.h"
#include "effect_draw.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "ffx_display_sites.h"
#include "ffx_observe.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <deque>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace ds3native::effect_system {
namespace {
Engine engine_copy{};const Engine* engine=&engine_copy;   // set by value at each entry point (review)
// One device token and one handle range per process (reserved at the first Build, never committed).
std::atomic<uintptr_t> device_token{},handle_base{},display_object{};
bool EnsureTokens() noexcept {
    static std::once_flag once;
    std::call_once(once,[]{
        const auto token=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,token_bytes,MEM_RESERVE,PAGE_NOACCESS));
        const auto handles=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,handle_bytes,MEM_RESERVE,PAGE_NOACCESS));
        if(token && handles){handle_base.store(handles);device_token.store(token);}});
    return device_token.load()!=0;
}
struct ObjectRecord {uint32_t kind{};std::array<uint32_t,8> description{};std::vector<uint8_t> bytes;uintptr_t parent{};uint64_t serial{};};   // kind 1 buffer, 2 shader view, 3 unordered view, 4 programme; serial: unique per creation (E07)
std::mutex buffer_lock;
std::vector<ObjectRecord> buffer_records;
uint64_t buffer_bytes{},released_records{};
std::deque<size_t> free_slots;
uintptr_t Rva(uint32_t rva) {return engine->image+rva;}
template<class R=uintptr_t,class... A> R Call(uint32_t rva,A... a) {return reinterpret_cast<R(__fastcall*)(A...)>(Rva(rva))(a...);}
template<class R=uintptr_t,class... A> R Virtual(uintptr_t object,uint32_t slot,A... a) {
    const auto table=*reinterpret_cast<const uintptr_t*>(object);
    return reinterpret_cast<R(__fastcall*)(uintptr_t,A...)>(*reinterpret_cast<const uintptr_t*>(table+slot))(object,a...);}
void Q(uintptr_t at,uint64_t v) {*reinterpret_cast<uint64_t*>(at)=v;}
void D(uintptr_t at,uint32_t v) {*reinterpret_cast<uint32_t*>(at)=v;}
void W(uintptr_t at,uint16_t v) {*reinterpret_cast<uint16_t*>(at)=v;}
void B(uintptr_t at,uint8_t v) {*reinterpret_cast<uint8_t*>(at)=v;}
uint64_t Qr(uintptr_t at) {return *reinterpret_cast<const uint64_t*>(at);}
[[noreturn]] void Refuse(const char* step) {throw std::runtime_error(step);}
uintptr_t Allocate(size_t size,size_t alignment,uintptr_t heap) {
    const auto storage=Call(0x1777A00,size,alignment,heap);
    if(!storage)Refuse("effect_system_allocation");
    return storage;
}
// The heap capability assertion every list constructor of these classes makes (vt+0x18, bit 5).
void HeapCapability(uintptr_t heap) {
    alignas(16) uint64_t scratch[2]{};
    const auto caps=Virtual<const uint32_t*>(heap,0x18,reinterpret_cast<uintptr_t>(scratch),0u);
    if(!caps || !((*caps>>5)&1))Refuse("effect_system_heap_capability");
}
// The engine's 4x4 identity: rows at 143D1BF90, 143D1BFA0, 143D1BFB0 and 143D1C080 (24B260 at +0x250 and +0x5F0).
void Identity(uintptr_t at) {
    for(const auto [row,source]:{std::pair{0u,0x3D1BF90u},{1u,0x3D1BFA0u},{2u,0x3D1BFB0u},{3u,0x3D1C080u}})
        std::memcpy(reinterpret_cast<void*>(at+16*row),reinterpret_cast<const void*>(Rva(source)),16);
}
// (mask & a) | (~mask & b), four dwords (24C530's blends of the scene entity's two mask vectors).
void Blend(uintptr_t at,uint32_t mask,uint32_t a,uint32_t b) {
    for(uint32_t i=0;i<4;++i){
        const auto m=*reinterpret_cast<const uint32_t*>(Rva(mask)+4*i);
        D(at+4*i,(m&*reinterpret_cast<const uint32_t*>(Rva(a)+4*i))|(~m&*reinterpret_cast<const uint32_t*>(Rva(b)+4*i)));}
}
// 24B260's type table: type, then the eight descriptor slots +0x08..+0x40 as the original writes them (0 = not written as a
// function; every slot is written, so a reused descriptor carries nothing over).
struct TypeRow {uint16_t type;std::array<uint32_t,8> slots;};
constexpr TypeRow types[]{
    {0x2710,{0x24A840,0x269020,0x2690B0,0x268E70,0x268FE0,0,0,0x2690F0}},
    {0x2712,{0x24A860,0x26EDB0,0x26F030,0x26EC30,0x26ED70,0,0,0x26F070}},
    {0x2713,{0x24A940,0x2710E0,0x271160,0x270F80,0x2710A0,0,0,0x2711A0}},
    {0x27D8,{0x24A880,0x275300,0x2753C0,0x275190,0x2752C0,0,0,0x275400}},
    {0x283C,{0x24A920,0x275F60,0x276230,0x275DF0,0x275F20,0,0,0x276270}},
    {0x2718,{0x24A960,0x277640,0x2776D0,0x277490,0x277600,0,0,0x277710}},
    {0x2968,{0x24A9C0,0,0,0x27BB70,0x27BC70,0,0,0x27BCB0}},
    {0x283D,{0x24A980,0x27C0B0,0x27C2A0,0x27BF40,0x27C070,0,0,0x27C2E0}},
    {0x283E,{0x24A9A0,0x27D3E0,0x27D480,0x27D270,0x27D3A0,0,0,0x27D4C0}},
    {0x025D,{0x2496B0,0,0,0x27E5E0,0x27E740,0,0x2575E0,0}},
    {0x271C,{0x249930,0,0,0x288C80,0x288D80,0,0,0x288DA0}},
    {0x0261,{0x24A9E0,0,0,0,0,0x28F200,0,0}},
    {0x2AF8,{0x24AA10,0,0,0,0,0x28F290,0,0}},
    {0x2AFA,{0x24AA30,0,0,0,0,0x28F320,0,0}}};
// Two property types (253990 into the property registry 144965290): only the slots +0x08..+0x20 are written; the original's
// stack descriptor keeps the last type's +0x28..+0x40 (0x2AFA: 0, 28F320, 0, 0), reproduced by reusing the same buffer.
struct PropertyRow {uint16_t type;std::array<uint32_t,4> slots;};
constexpr PropertyRow properties[]{{0x4B,{0x249760,0,0,0x25F8C0}},{0x51,{0x249780,0,0,0x25F910}}};

// 24B260 without its five GX constant-buffer creations (the members +0xE8..+0x108 stay null) and without the two parts the
// native startup already built (1A97EE0 at archive state 4; 255090 and the node manager in PublishNativeFxWorld).
void SceneControl(uintptr_t sc,uintptr_t heap,uint32_t ring,uintptr_t param4,uint32_t param5,uintptr_t token) {
    Q(sc,Rva(0x3D64008));
    Q(sc+0x08,0);Q(sc+0x10,heap);Q(sc+0x18,0);W(sc+0x20,1);Q(sc+0x28,0);
    Q(sc+0x30,~0ull);Q(sc+0x38,~0ull);D(sc+0x40,0xFFFFFFFF);B(sc+0x44,0);
    Q(sc+0x50,0);Q(sc+0x58,0);Q(sc+0x60,sc+0x50);Q(sc+0x68,0);Q(sc+0x70,0);Q(sc+0x78,0);Q(sc+0x80,0);B(sc+0x88,1);
    HeapCapability(heap);
    Q(sc+0x90,0);Q(sc+0x98,0);Q(sc+0xA0,0);Q(sc+0xA8,heap);Q(sc+0xB0,Rva(0x473F7F0));D(sc+0xB8,ring);
    Q(sc+0xC0,0);Q(sc+0xC8,0);Q(sc+0xD0,0);D(sc+0xD8,0);
    for(uint32_t o=0xE0;o<=0x108;o+=8)Q(sc+o,0);
    Call<void>(0x266BB0,sc+0x110,heap);
    Q(sc+0x148,0);
    for(const uint32_t target:{0x150u,0x1B0u}){   // the two GXRenderToTexture shells, fields only
        Q(sc+target,Rva(0x3D4BAA0));Q(sc+target+8,heap);W(sc+target+0x10,0);B(sc+target+0x12,0);
        for(uint32_t o=0x18;o<=0x58;o+=8)Q(sc+target+o,0);}
    D(sc+0x210,0);
    for(uint32_t o=0x218;o<=0x248;o+=8)Q(sc+o,0);
    Identity(sc+0x250);
    B(sc+0x290,0);
    Call<uintptr_t>(0x6E780,sc+0x2A0);
    D(sc+0x580,param5);Q(sc+0x588,param4);
    HeapCapability(Qr(sc+0x10));
    Q(sc+0x598,0);Q(sc+0x5A0,0);Q(sc+0x5A8,0);Q(sc+0x5B0,Qr(sc+0x10));
    Call<uintptr_t>(0x253150,sc+0x638);
    HeapCapability(heap);
    // The unordered map at +0x660: its list (sentinel from 253390), 16 buckets of the sentinel, mask 7, size 8, load 1.0.
    Q(sc+0x660,0);Q(sc+0x668,0);Q(sc+0x670,heap);
    Q(sc+0x660,Call(0x253390,sc+0x660,uintptr_t{0},uintptr_t{0}));
    Q(sc+0x678,0);Q(sc+0x680,0);Q(sc+0x688,0);Q(sc+0x690,heap);D(sc+0x6A8,0x3F800000);
    {uint64_t sentinel=Qr(sc+0x660);uint64_t out{};Q(sc+0x680,Qr(sc+0x678));
        Call<uintptr_t>(0x2533F0,sc+0x678,reinterpret_cast<uintptr_t>(&out),Qr(sc+0x678),uint64_t{0x10},reinterpret_cast<uintptr_t>(&sentinel));}
    Q(sc+0x698,7);Q(sc+0x6A0,8);
    B(sc+0x5E3,0);B(Rva(0x496237C),0);B(sc+0x590,0);
    if(!Qr(sc+0x588))Q(sc+0x588,Qr(sc+0x10));
    if(!Qr(Rva(0x4929E28)))Q(Rva(0x4929E28),Call(0x1788CF0));
    Call<void>(0x25E7F0,Qr(Rva(0x4929E28)));
    Q(Rva(0x49623C0),sc);
    // Five FUN_1419254F0(*[GX], ...) constant buffers on the device token: native records (CreateBuffer), their handles stored
    // where the original stores its GX buffers. Description {size, 0, 4 (constant buffer), 0, 0x800, 0}, no initial data.
    {alignas(16) std::array<uint32_t,6> description{0xE0,0,4,0,0x800,0};
        for(const auto [member,size]:{std::pair{0x100u,0xE0u},{0xE8u,0x1C0u},{0xF0u,0x1C0u},{0xF8u,0x230u},{0x108u,0x190u}}){
            description[0]=size;
            if(Call<uint32_t>(0x19254F0,token,sc+member,reinterpret_cast<uintptr_t>(description.data()),uintptr_t{0},uintptr_t{0},uintptr_t{0},uintptr_t{0}))
                Refuse("effect_system_constant_buffer");}}
    B(sc+0x5C6,0);D(sc+0x5BB,0);D(sc+0x5D0,0x3F800000);D(sc+0x5CC,0);W(sc+0x5B9,0);W(sc+0x5C0,0);W(sc+0x5E0,1);
    B(sc+0x5B8,1);B(sc+0x5BF,0);B(sc+0x5E2,0);
    Identity(sc+0x5F0);
    D(sc+0x630,0x41200000);
    {   // The list at +0x638 replaced by a fresh empty one (24B260 14024B85F..14024B92F).
        const auto list_heap=Qr(sc+0x10);HeapCapability(list_heap);
        std::array<uint64_t,3> fresh{0,0,list_heap};
        const auto sentinel=Call(0x253330,reinterpret_cast<uintptr_t>(fresh.data()),uintptr_t{0},uintptr_t{0});
        fresh[0]=sentinel;
        const auto own_heap=Qr(sc+0x648);
        Virtual<uintptr_t>(list_heap,0x08);Virtual<uintptr_t>(own_heap,0x08);
        Call<uintptr_t>(0x2541C0,sc+0x638,Qr(sentinel),sentinel);
        auto node=Qr(sentinel);Q(sentinel,sentinel);Q(sentinel+8,sentinel);
        while(node!=sentinel){const auto next=Qr(node);Call<void>(0x1A879F0,node+0x10);Virtual<void>(list_heap,0x68,node);node=next;}
        Virtual<void>(list_heap,0x68,sentinel);
    }
    D(sc+0x5C2,0);D(sc+0x5C8,1);B(sc+0x658,0);D(sc+0x650,0xFFFFFFFF);B(sc+0x6B0,0);B(sc+0x5E4,1);D(sc+0x654,0);
    Q(sc+0x70,sc);
    D(sc+0x3C,0x20);D(sc+0x30,0x21);D(sc+0x38,0x2D);D(sc+0x40,0x31);D(sc+0x34,0x29);
    if(!*reinterpret_cast<const uint8_t*>(sc+0x44)){
        // 1A97EE0(heap, heap) ran at archive state 4 (NativeSfxStartup); the registrations and seals follow here.
        Call<uintptr_t>(0x36D9A0);
        alignas(16) std::array<uint64_t,9> desc{};
        for(const auto& row:types){
            *reinterpret_cast<uint16_t*>(&desc[0])=row.type;
            for(size_t i=0;i<8;++i)desc[1+i]=row.slots[i]?Rva(row.slots[i]):0;
            const auto heap_now=Qr(sc+0x10);
            if(Call<uint8_t>(0x253710,uintptr_t{0},reinterpret_cast<uintptr_t>(desc.data()),heap_now)){
                alignas(16) std::array<uint64_t,3> second{row.type,Rva(0x253670),Rva(0x2536F0)};
                Call<uintptr_t>(0x1AB3A60,reinterpret_cast<uintptr_t>(second.data()),heap_now);}
        }
        for(const auto& row:properties){
            *reinterpret_cast<uint16_t*>(&desc[0])=row.type;
            for(size_t i=0;i<4;++i)desc[1+i]=row.slots[i]?Rva(row.slots[i]):0;
            const auto heap_now=Qr(sc+0x10);
            if(Call<uint8_t>(0x253990,Rva(0x4965290),reinterpret_cast<uintptr_t>(desc.data()),heap_now)){
                alignas(16) std::array<uint64_t,3> second{row.type,Rva(0x2538E0),Rva(0x253970)};
                Call<uintptr_t>(0x1AB3A60,reinterpret_cast<uintptr_t>(second.data()),heap_now);}
        }
        Call<uintptr_t>(0x36DB30);Call<uintptr_t>(0x1A98500);
        B(sc+0x44,1);
    }
    // The resource manager with the device token in place of *[144961AB0] (28F4D0 only stores it, at +0x08 and +0x178).
    const auto resources=Allocate(0x198,8,Qr(sc+0x10));
    Q(sc+0x28,Call(0x28F4D0,resources,token,Qr(sc+0x10),sc));
    // 255090 (light registry and FX world) and the node manager 25B280/25B370: published by PublishNativeFxWorld.
}
// 24C530 without the GPU ring pool: sc+0xC0 = sc+0xC8 = null, one context (sc+0xD8 = 1) with a null sub-allocator slot.
void DeviceInitialisation(uintptr_t sc,uintptr_t token) {
    Q(sc+0xC0,0);Q(sc+0xC8,0);D(sc+0xD8,1);
    const auto slots=Virtual(Qr(sc+0x10),0x50,uint64_t{8},uint64_t{0x10});
    if(!slots)Refuse("effect_system_context_slots");
    Q(slots,0);Q(sc+0xD0,slots);
    uintptr_t manager=Allocate(0x28,8,Qr(sc+0x10));
    Call<uintptr_t>(0x2155140,manager);
    Q(manager,Rva(0x3D6A078));Q(manager+0x18,token);Q(manager+0x20,sc);
    Q(sc+0x18,manager);
    Call<uintptr_t>(0x2155700,manager,uint32_t{0x1E},uint32_t{3});
    const auto entity=Allocate(0x80,0x10,Qr(sc+0x10));
    std::memset(reinterpret_cast<void*>(entity),0,0x80);
    D(entity+0x08,0);Q(entity+0x18,0);D(entity+0x20,1);D(entity+0x24,1);Q(entity+0x28,0);Q(entity+0x30,0);
    Q(entity,Rva(0x2947908));Q(entity+0x10,Rva(0x2947938));
    Blend(entity+0x40,0x39F2650,0x3D1C120,0x3D7F7F0);
    Blend(entity+0x50,0x39F2650,0x3D1C120,0x3D7F750);
    Q(entity+0x60,0);Q(entity+0x68,0);
    Q(entity,Rva(0x3D63F90));Q(entity+0x10,Rva(0x3D63FC0));Q(entity+0x70,sc);
    if(Qr(sc+0x08))Refuse("effect_system_entity_present");
    Q(sc+0x08,entity);D(entity+0x08,1);
    Q(entity+0x20,0x100000011ull);   // +0x18 (the scene) is null: no vt+0x28 notification
}
}

bool IsToken(uintptr_t address,const System&) noexcept {
    const auto token=device_token.load(),handles=handle_base.load(),display=display_object.load();
    return (token && address>=token && address<token+token_bytes) || (handles && address>=handles && address<handles+handle_bytes) ||
        (display && address>=display && address<display+0x1000);
}
uintptr_t Display() noexcept {return display_object.load();}
Redirect RedirectDisplay(uintptr_t image,bool draw_side) noexcept {
    Redirect result;
    const auto refuse=[&](const char* why){result.refused=why;return result;};
    if(display_object.load())return refuse("display_redirect_repeated");
    if(!EnsureTokens())return refuse("display_tokens");
    // Every site first: its original bytes, a 7-byte mov reg,[rip+disp32] reaching 144961AB0.
    const uintptr_t display_global=image+0x4961AB0;
    uintptr_t low=~uintptr_t{},high=0;
    for(const auto& site:display_sites){
        const auto at=image+site.rva;
        if(std::memcmp(reinterpret_cast<const void*>(at),site.bytes,7))return refuse("display_site_bytes");
        int32_t disp{};std::memcpy(&disp,site.bytes+3,4);
        if(at+7+static_cast<intptr_t>(disp)!=display_global)return refuse("display_site_target");
        low=std::min(low,at);high=std::max(high,at+7);
    }
    // The effect display: one read-only page, zero (the GX feature absent, as natively) but for the fields the module's
    // GX statements need: +0x000 the device (the device token), +0x2B8 the deferred-release queue (a token address: the
    // release FUN_1400D8780 is native for effect objects and never reads it). A write to the display faults.
    const auto display=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(!display)return refuse("display_object");
    *reinterpret_cast<uintptr_t*>(display)=device_token.load();
    *reinterpret_cast<uintptr_t*>(display+0x2B8)=device_token.load()+0x2B8;
    // +0x60: the context array whose first context's +8 is the main context (FUN_140296E20/296690 store it in the update and
    // proceed passes for the draw-side work, which does not run): a token address.
    *reinterpret_cast<uintptr_t*>(display+0x60)=display+0x800;
    *reinterpret_cast<uintptr_t*>(display+0x800)=display+0x810;
    *reinterpret_cast<uintptr_t*>(display+0x818)=device_token.load()+0x100;
    // FFX-DRAW N5b: the draw side's slot 27 (264230) reads the state bank +0x2A0 (blend, depth, raster, sampler objects) and,
    // for the default-texture rule, the provider +0x270: native tokens (effect_draw), which the GX wrapper capture records.
    if(draw_side){*reinterpret_cast<uintptr_t*>(display+0x270)=effect_draw::DefaultTextureProvider();
        *reinterpret_cast<uintptr_t*>(display+0x2A0)=effect_draw::StateBank();
        // E07: +0x288, the unbinding set the GPU-particle simulation reads after its dispatches (2A7A09: three null views): zeros.
        static const std::array<uintptr_t,16> nulls{};*reinterpret_cast<uintptr_t*>(display+0x288)=reinterpret_cast<uintptr_t>(nulls.data());}
    DWORD previous{};
    if(!VirtualProtect(reinterpret_cast<void*>(display),0x1000,PAGE_READONLY,&previous))return refuse("display_object_protect");
    // The slot within reach of every site's 32-bit displacement: the first free 64 KB block after the image.
    MEMORY_BASIC_INFORMATION region{};uintptr_t slot{};
    for(uintptr_t probe=(high+0xFFFF)&~uintptr_t{0xFFFF};probe<low+0x7FF00000 && !slot;probe+=0x10000){
        if(!VirtualQuery(reinterpret_cast<void*>(probe),&region,sizeof(region)))break;
        if(region.State!=MEM_FREE){probe=((reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize+0xFFFF)&~uintptr_t{0xFFFF})-0x10000;continue;}
        slot=reinterpret_cast<uintptr_t>(VirtualAlloc(reinterpret_cast<void*>(probe),0x1000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    }
    if(!slot)return refuse("display_slot");
    *reinterpret_cast<uintptr_t*>(slot)=display;
    if(!VirtualProtect(reinterpret_cast<void*>(slot),0x1000,PAGE_READONLY,&previous))return refuse("display_slot_protect");
    for(const auto& site:display_sites){
        const auto at=image+site.rva;
        const auto disp=static_cast<intptr_t>(slot)-static_cast<intptr_t>(at+7);
        if(disp<INT32_MIN || disp>INT32_MAX)return refuse("display_slot_reach");
    }
    // Every code page made writable first (review: all or nothing), then the displacements, then the protections restored.
    std::vector<std::pair<uintptr_t,DWORD>> pages_made_writable;
    for(uintptr_t page=low&~uintptr_t{0xFFF};page<high;page+=0x1000){DWORD protection{};
        if(!VirtualProtect(reinterpret_cast<void*>(page),0x1000,PAGE_EXECUTE_READWRITE,&protection)){
            for(const auto& [restore,old]:pages_made_writable){DWORD ignored{};VirtualProtect(reinterpret_cast<void*>(restore),0x1000,old,&ignored);}
            return refuse("display_site_protect");}
        pages_made_writable.emplace_back(page,protection);}
    for(const auto& site:display_sites){
        const auto at=image+site.rva;
        const auto disp=static_cast<int32_t>(static_cast<intptr_t>(slot)-static_cast<intptr_t>(at+7));
        std::memcpy(reinterpret_cast<void*>(at+3),&disp,4);
        ++result.sites;
    }
    for(const auto& [restore,old]:pages_made_writable){DWORD ignored{};VirtualProtect(reinterpret_cast<void*>(restore),0x1000,old,&ignored);}
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(low),high-low);
    display_object.store(display);
    result.slot=slot;result.display=display;
    return result;
}
bool IsEffectDevice(uintptr_t device) noexcept {const auto token=device_token.load();return token && device==token;}
// Under buffer_lock: a released slot is reused only once 1024 others wait behind it (review: the handle range holds 65,535 slots;
// the delay keeps a stale handle from naming a new object soon after its release).
bool Store(ObjectRecord&& record,uintptr_t out) {
    static uint64_t serials{};record.serial=++serials;   // under buffer_lock
    size_t index;
    if(free_slots.size()>1024){index=free_slots.front();free_slots.pop_front();buffer_records[index]=std::move(record);}
    else{if((buffer_records.size()+1)*handle_stride>=handle_bytes)return false;
        index=buffer_records.size();buffer_records.push_back(std::move(record));}
    *reinterpret_cast<uintptr_t*>(out)=handle_base.load()+handle_stride*(index+1);
    return true;
}
uint32_t CreateBuffer(uintptr_t out,const uint32_t* d,uintptr_t initial) noexcept {
    // FUN_1419254F0's own description check.
    const uint32_t size=d[0],misc=d[4],stride=d[5];
    const bool structured_ok=!(misc&0x20) || (stride && stride<=size && size%stride==0 && !(stride&3) && stride<=0x800);
    if(!out || !size || (misc&0x87C5) || (d[2]&0x40) || !structured_ok)return 8;
    ObjectRecord record;record.kind=1;std::memcpy(record.description.data(),d,sizeof(uint32_t)*6);
    if(initial){const auto bytes=*reinterpret_cast<const uint8_t* const*>(initial);
        if(bytes)record.bytes.assign(bytes,bytes+size);}
    const std::lock_guard hold(buffer_lock);
    if(!Store(std::move(record),out))return 8;
    buffer_bytes+=size;
    return 0;
}
BufferStats Buffers() noexcept {const std::lock_guard hold(buffer_lock);return {buffer_records.size(),buffer_bytes,released_records};}
bool Release(uintptr_t handle) noexcept {
    const auto base=handle_base.load();
    if(!base || handle<=base || handle>=base+handle_bytes || (handle-base)%handle_stride)return false;
    const std::lock_guard hold(buffer_lock);
    const size_t index=(handle-base)/handle_stride-1;
    if(index>=buffer_records.size() || !buffer_records[index].kind)return false;
    if(buffer_records[index].kind==1)buffer_bytes-=buffer_records[index].description[0];
    buffer_records[index]=ObjectRecord{};++released_records;free_slots.push_back(index);
    return true;
}
bool IsEffectObject(uintptr_t handle) noexcept {
    const auto base=handle_base.load();
    return base && handle>base && handle<base+handle_bytes && !((handle-base)%handle_stride);
}
uint64_t Serial(uintptr_t handle) noexcept {
    if(!IsEffectObject(handle))return 0;
    const std::lock_guard hold(buffer_lock);
    const size_t index=(handle-handle_base.load())/handle_stride-1;
    return index<buffer_records.size() && buffer_records[index].kind?buffer_records[index].serial:0;
}
uintptr_t ViewParent(uintptr_t handle) noexcept {
    if(!IsEffectObject(handle))return 0;
    const std::lock_guard hold(buffer_lock);
    const size_t index=(handle-handle_base.load())/handle_stride-1;
    return index<buffer_records.size() && (buffer_records[index].kind==2 || buffer_records[index].kind==3)?buffer_records[index].parent:0;
}
bool BufferBytes(uintptr_t handle,std::vector<uint8_t>& out) {
    if(!IsEffectObject(handle))return false;
    const std::lock_guard hold(buffer_lock);
    const size_t index=(handle-handle_base.load())/handle_stride-1;
    if(index>=buffer_records.size() || buffer_records[index].kind!=1)return false;
    out=buffer_records[index].bytes;return true;
}
bool Describe(uintptr_t handle,uint32_t& kind,std::array<uint32_t,8>& description) noexcept {
    if(!IsEffectObject(handle))return false;
    const std::lock_guard hold(buffer_lock);
    const size_t index=(handle-handle_base.load())/handle_stride-1;
    if(index>=buffer_records.size() || !buffer_records[index].kind)return false;
    kind=buffer_records[index].kind;description=buffer_records[index].description;return true;
}
uint32_t CreateProgram(uintptr_t out,uintptr_t bytes,uint64_t size) noexcept {
    if(!out || !bytes || !size || size>(16u<<20))return 8;
    ObjectRecord record;record.kind=4;record.bytes.resize(static_cast<size_t>(size));
    if(!engine->read || !engine->read(bytes,record.bytes.data(),record.bytes.size()))return 8;   // guarded (the engine's Read)
    record.description[0]=static_cast<uint32_t>(size);
    // FFX-DRAW N6: the source address too (+1 low, +2 high): for the shader set's input layouts (1925210 is CreateInputLayout, its
    // "bytecode" the GX element table in the image and "size" its element count), the table the native layout is decoded from.
    record.description[1]=static_cast<uint32_t>(bytes);record.description[2]=static_cast<uint32_t>(static_cast<uint64_t>(bytes)>>32);
    const std::lock_guard hold(buffer_lock);
    return Store(std::move(record),out)?0:8;
}
uint32_t CreateView(uint32_t kind,uintptr_t out,uintptr_t resource,const uint32_t* d) noexcept {
    if(!out || !IsEffectObject(resource) || !d)return 8;
    ObjectRecord record;record.kind=kind;record.parent=resource;std::memcpy(record.description.data(),d,kind==3?20:24);   // the UAV (20) and SRV (24) descriptions (review: never read past them)
    const std::lock_guard hold(buffer_lock);
    return Store(std::move(record),out)?0:8;
}
System Build(const Engine& e,uintptr_t manager) {
    engine_copy=e;
    System s;s.manager=manager;
    uintptr_t heap{};
    if(!e.read(Rva(0x479C270),&heap,8) || !heap)Refuse("effect_system_heap");
    for(const uint32_t member:{0x38u,0x40u,0x48u,0x50u,0x58u})if(Qr(manager+member))Refuse("effect_system_manager_not_pristine");
    uintptr_t prior{};
    if(!e.read(Rva(0x49623C0),&prior,8) || prior)Refuse("effect_system_scene_control_published");
    if(!EnsureTokens())Refuse("effect_system_token");
    s.token=device_token.load();
    // E6C9D0 steps 1-4: shader container, texture repository, FLVER container, sound manager.
    {const auto a=Allocate(0x30,8,heap);Q(manager+0x38,Call(0xD53340,a));}
    {const auto a=Allocate(8,8,heap);Q(manager+0x40,Call(0xD53270,a));}
    {const auto a=Allocate(0x58,8,heap);Q(manager+0x48,Call(0xD53100,a));}
    {const auto a=Allocate(8,8,heap);Q(manager+0x50,Call(0xD538A0,a));}
    // Step 5: the scene control (E6C9D0 stores +0x58 here; natively the store waits for FW-3's publish).
    const auto sc=Allocate(0x6C0,0x10,heap);
    std::memset(reinterpret_cast<void*>(sc),0,0x6C0);
    SceneControl(sc,heap,0x200000,0,0x80000,s.token);
    // Step 6, 24C300: the shader set FUN_140291520 on the device token (its programme creations FUN_141925210/19250A0 become
    // native programme records keeping the bytecode), stored at sc+0xE0 and [sc+0x28]+0x168 (Native1486: the GPU-particle
    // instance creation reads it).
    {std::optional<effect_draw::ProgramScope> programmes;if(e.program_tokens)programmes.emplace();   // FFX-DRAW N6: the programmes as native tokens
        const auto set=Call(0x291520,s.token,Qr(sc+0x10),Qr(manager+0x38));programmes.reset();if(!set)Refuse("effect_system_shader_set");   // review: 0 on a failed allocation or no container
        Q(sc+0xE0,set);Q(Qr(sc+0x28)+0x168,set);}
    Call<void>(0x24C390,sc,Qr(manager+0x40));
    Call<void>(0x24C3A0,sc,Qr(manager+0x48));
    Q(Rva(0x4962380),Rva(0xE6B950));B(Rva(0x4962377),1);B(sc+0x5E0,0);B(sc+0x590,0);
    // 144962376 (multi-threaded update) is the single-threaded branch natively; 14473DAD7 is read only by the other branch.
    B(Rva(0x4962376),0);
    DeviceInitialisation(sc,s.token);
    Q(sc+0xB0,Qr(manager+0x50));
    B(Rva(0x496237C),1);D(Rva(0x4962378),0);Q(Rva(0x4962388),Rva(0xE6FC40));Q(Rva(0x4962390),Rva(0xE6FBF0));
    s.scene_control=sc;s.ffx_manager=Qr(sc+0x18);s.entity=Qr(sc+0x08);s.resources=Qr(sc+0x28);
    return s;
}
bool Check(const Engine& e,const System& s) {
    engine_copy=e;
    const auto sc=s.scene_control;
    uint32_t ids[5]{};std::memcpy(ids,reinterpret_cast<const void*>(sc+0x30),sizeof(ids));
    uint8_t gx_sealed{},registered=*reinterpret_cast<const uint8_t*>(sc+0x44);
    e.read(Rva(0x49651E8),&gx_sealed,1);
    // The GX type registry [1449651D0]: 0x48-byte entries, the type in the first word, the count at [1449651D8].
    uintptr_t table{};uint64_t count{};e.read(Rva(0x49651D0),&table,8);e.read(Rva(0x49651D8),&count,8);
    int present=0;
    for(const uint16_t type:{uint16_t{0x27D8},uint16_t{0x283C},uint16_t{0x283D},uint16_t{0x283E},uint16_t{0x2713},uint16_t{0x261},uint16_t{0x2AF8}})
        for(uint64_t i=0;table && i<count && i<4096;++i){uint16_t t{};if(e.read(table+0x48*i,&t,2) && t==type){++present;break;}}
    uintptr_t published{};e.read(Rva(0x49623C0),&published,8);
    const bool ok=Qr(sc)==Rva(0x3D64008) && Qr(sc+0x70)==sc && ids[0]==0x21 && ids[1]==0x29 && ids[2]==0x2D && ids[3]==0x20 &&
        ids[4]==0x31 && registered==1 && *reinterpret_cast<const uint32_t*>(sc+0xB8)==0x200000 && gx_sealed && present==7 &&
        published==sc && s.ffx_manager && Qr(s.ffx_manager+0x18)==s.token && s.entity && s.resources && Qr(sc+0xE0) && Qr(s.resources+0x168)==Qr(sc+0xE0);
    char row[512]{};sprintf_s(row,"{\"event\":\"native_effect_system_built\",\"ok\":%s,\"scene_control\":%llu,\"pass_ids\":[%u,%u,%u,%u,%u],"
        "\"types_registered\":%u,\"gx_registry_sealed\":%u,\"gx_types\":%llu,\"wave1_types_present\":%d,\"ring\":%u,\"token\":%llu,"
        "\"native_objects\":%llu,\"native_buffer_bytes\":%llu,\"effect_system_published\":%s}\n",ok?"true":"false",static_cast<unsigned long long>(sc),
        ids[0],ids[1],ids[2],ids[3],ids[4],registered,gx_sealed,static_cast<unsigned long long>(count),present,*reinterpret_cast<const uint32_t*>(sc+0xB8),
        static_cast<unsigned long long>(s.token),static_cast<unsigned long long>(Buffers().buffers),static_cast<unsigned long long>(Buffers().bytes),
        s.published?"true":"false");
    e.log(row);
    return ok;
}
namespace {
UpdateStats update_stats{};
std::mutex kinds_lock;std::map<uint32_t,uint64_t> proceed_kinds,update_kinds;   // ItemKinds
// The update pass (0x2D50), the proceed pass and the combined pass of one slot-8 call: the original keeps them on its stack,
// constructed and destroyed within the call; slot 8 runs on one thread.
alignas(16) std::array<uint8_t,0x3000> update_pass{},proceed_pass{};
alignas(16) std::array<uint8_t,0x100> combined_pass{};
float F(uintptr_t at) {return *reinterpret_cast<const float*>(at);}
// I-0092: body(i) for i in [0, count), as DS3's ranged GX jobs run their lists (FUN_1400B6480: ranges of min(count, 10), the
// GX workers and the caller taking them): by tickets of 10 on the pool, the caller helping; on the calling thread without a
// pool or with one range. Returns once every range has run.
template<class Body> void RunRanges(WorkPool* pool,size_t count,const Body& body) {
    constexpr size_t range=10;
    if(!pool || count<=range){for(size_t i=0;i<count;++i)body(i);return;}
    std::atomic<size_t> next{};
    const std::function<void(size_t)> job=[&](size_t){
        for(size_t begin;(begin=next.fetch_add(range,std::memory_order_relaxed))<count;)
            for(size_t i=begin;i<begin+range && i<count;++i)body(i);};
    pool->Run(job);
}
}
uint32_t UpdateNodes(const Engine& e,uintptr_t mgr,float dt) {
    engine_copy=e;
    // (1) the debug hold, (2) the busy flag.
    if(*reinterpret_cast<const uint8_t*>(mgr+0x45)){
        if(!*reinterpret_cast<const int32_t*>(mgr+0x48))B(mgr+0x45,0);
        else D(mgr+0x48,*reinterpret_cast<const uint32_t*>(mgr+0x48)-1);}
    B(mgr+0x44,0);
    LARGE_INTEGER start{};QueryPerformanceCounter(&start);
    // (3) the node pointers of the list [mgr+0x10] (the node at entry+0x10), copied under the manager's lock (+0x58: vt+0x10
    // lock with an infinite wait, vt+0x20 unlock), which node creation FUN_14025B610 and release FUN_14025B750 take.
    std::vector<uintptr_t> nodes;
    Virtual<uintptr_t>(mgr+0x58,0x10,0xFFFFFFFFu);
    const auto head=Qr(mgr+0x10);
    for(auto entry=Qr(head);entry!=head;entry=Qr(entry))nodes.push_back(Qr(entry+0x10));
    Virtual<uintptr_t>(mgr+0x58,0x20);
    // (4) the job body FUN_14025B1F0 over [0, [mgr+0x30]): I-0092, by DS3's ranges on the pool (25B870's ranged job).
    const auto live=*reinterpret_cast<const uint32_t*>(mgr+0x30);
    const size_t count=std::min<size_t>(live,nodes.size());
    RunRanges(e.pool,count,[&](size_t i){
        const auto node=nodes[i];
        const auto callback=Qr(node+0x170);
        if(callback && Qr(node+0x160))reinterpret_cast<void(__fastcall*)(float)>(callback)(dt);   // 25B1F0: MOVSS XMM0,[job+0x28]
        Call<void>(0x25C700,node);});
    // (5) the elapsed time in milliseconds at +0x50, as the original computes it (counter delta / [144929E00] * [143D7F040]).
    LARGE_INTEGER end{};QueryPerformanceCounter(&end);
    const auto frequency=*reinterpret_cast<const int64_t*>(Rva(0x4929E00));
    if(frequency)*reinterpret_cast<float*>(mgr+0x50)=static_cast<float>(static_cast<double>(end.QuadPart-start.QuadPart)/
        static_cast<double>(frequency))*F(Rva(0x3D7F040));
    update_stats.nodes=count;update_stats.node_refills+=count;
    ffx_observe::Note(ffx_observe::NodeUpdate,0,mgr,0,0,0);   // review: the observer keeps counting the transcribed update (FF3)
    return static_cast<uint32_t>(count);
}
void Update(const Engine& e,uintptr_t sc,float dt) {
    engine_copy=e;
    // Environment factors from the scene root [144961B50] (+0x5360 the environment record, +0x5368 the factors).
    if(const auto root=Qr(Rva(0x4961B50));root && Qr(root+0x5360)){
        const auto env=Qr(root+0x5360);
        const auto flags=Qr(env+0x48);
        const float base4c=F(env+0x4C),raw50=F(env+0x50),v54=F(env+0x54),v58=F(env+0x58),v5c=F(env+0x5C),v60=F(env+0x60);
        const float one=F(Rva(0x3D7EE10));
        float f140=one,f1a0=one,f1d0=one,f1b8=one;
        if(const auto factors=Qr(root+0x5368)){f140=F(factors+0x140);f1a0=F(factors+0x1A0);f1d0=F(factors+0x1D0);f1b8=F(factors+0x1B8);}
        *reinterpret_cast<float*>(Rva(0x473E6EC))=f1a0;
        B(Rva(0x4962374),(0.f<f1a0 && ((flags>>8)&0xFF))?0:1);
        if(0.f<f1d0 && (flags&0xFF))B(Rva(0x496235D),0);
        B(Rva(0x496235E),(0.f<f1b8 && ((flags>>16)&0xFF))?0:1);
        const float k=F(Rva(0x3D7EDB4));
        const auto mix=[&](float v){const float scaled=v*k;return (v-scaled)*f140+scaled;};
        const float c4c=mix(base4c);
        float exact_half{};const uint32_t half_bits=0x3F00068E;std::memcpy(&exact_half,&half_bits,4);   // the floor 0x3F00068E
        using FloatCall=void(__fastcall*)(float);
        reinterpret_cast<FloatCall>(Rva(0x1ACAFC0))(F(Rva(0x3D7EDB8))<=c4c?c4c:exact_half);
        reinterpret_cast<FloatCall>(Rva(0x1ACAE60))(raw50);
        reinterpret_cast<FloatCall>(Rva(0x1ACAEA0))(mix(v54));
        reinterpret_cast<FloatCall>(Rva(0x1ACAEE0))(mix(v58));
        reinterpret_cast<FloatCall>(Rva(0x1ACAF20))(mix(v5c));
        reinterpret_cast<FloatCall>(Rva(0x1ACAF60))(mix(v60));
    }
    // The resource tick.
    if(const auto resources=Qr(sc+0x28);resources && Qr(sc+0x18))
        if(const auto tick=Qr(resources+0x160))Call<void>(0x2B7650,tick,Qr(Qr(sc+0x18)+8));
    // The hide mask.
    D(sc+0x14C,*reinterpret_cast<const uint32_t*>(sc+0x148));D(sc+0x148,0);
    // The sound object (+0x248; null natively: the colour table object D52CA0 is not built, I-0066).
    if(const auto sound=Qr(sc+0x248)){
        B(sound+0x10,*reinterpret_cast<const uint8_t*>(sc+0x5C5));
        const auto first=Qr(sound+0x18),last=Qr(sound+0x20);
        // 24D000 compares each bound with itself, so only this path is live: FUN_140255FD0(last, first, last), end = begin.
        Call<void>(0x255FD0,last,first,last);Q(sound+0x20,Qr(sound+0x18));
        Call<void>(0x255DF0,sound+0x48);
    }
    // The time scale, the registry controllers (vt+0x50 with dt), the node manager.
    dt*=F(sc+0x5D0);
    if(const auto registry=Qr(Rva(0x4962398))){
        const auto head=Qr(registry+0x20);
        for(auto entry=Qr(head);entry!=Qr(registry+0x20);entry=Qr(entry))
            if(const auto controller=Qr(Qr(entry+0x10)+0x48))Virtual<void>(controller,0x50,dt);
    }
    if(const auto nodes=Qr(Rva(0x49623A0)))UpdateNodes(e,nodes,dt);
    if(*reinterpret_cast<const uint8_t*>(Rva(0x496237E)))dt=0.f;
    if(!*reinterpret_cast<const uint8_t*>(sc+0x20))return;
    Q(sc+0x5A0,Qr(sc+0x598));   // the 0x60-byte entity vector emptied
    if(*reinterpret_cast<const int32_t*>(sc+0x654)>0){D(sc+0x654,0);B(Rva(0x496235C),1);}   // the GX flush FUN_14000B4E0: none natively
    if(Qr(sc+0x18)){
        // DAT_144962376 is 0 natively: the single-threaded branch.
        const auto update=reinterpret_cast<uintptr_t>(update_pass.data()),proceed=reinterpret_cast<uintptr_t>(proceed_pass.data()),
            combined=reinterpret_cast<uintptr_t>(combined_pass.data());
        update_pass.fill(0);proceed_pass.fill(0);combined_pass.fill(0);
        Call<uintptr_t>(0x296E20,update,Qr(sc+0x588),Qr(sc+0xC8),Qr(sc+0xE0));
        Call<uintptr_t>(0x296690,proceed,Qr(sc+0x588),Qr(sc+0xC8),Qr(sc+0xE0));
        Call<uintptr_t>(0x297110,combined,update,proceed,Qr(sc+0xC8),Qr(sc+0xE0),sc+0x30,Qr(sc+0x588),sc,
            static_cast<uint64_t>(*reinterpret_cast<const uint32_t*>(sc+0x580)));
        if(*reinterpret_cast<const uint8_t*>(sc+0x290)){B(combined+0xA0,1);Q(combined+0xA8,sc+0x2A0);}
        const auto manager=Qr(sc+0x18);
        Virtual<void>(manager,0x18,dt,combined);
        // The two work lists (GX jobs in the original, proceed then update; see the header): the proceed list counted by item
        // kind, the update list's particle simulation and message items run when e.run_cpu_items, its other items counted by kind.
        update_stats.proceed_items+=(Qr(proceed+0xA0)-Qr(proceed+0x98))>>3;
        update_stats.update_items+=(Qr(update+0x2D38)-Qr(update+0x2D30))>>4;
        if(e.run_cpu_items){   // review: only with the draw side (the default path is wave 1's)
            const auto particle=Rva(0x1ADC100),message=Rva(0x1ADC190),first=Qr(update+0x2D30),end=Qr(update+0x2D38);
            // I-0092: the kinds (a diagnostic) only at the update whose row is logged.
            if(e.count_kinds){const std::lock_guard hold(kinds_lock);
                for(auto bucket=Qr(proceed+0x98);bucket<Qr(proceed+0xA0);bucket+=8){const auto head=Qr(bucket);if(!head)continue;
                    for(auto link=Qr(head);link;link=Qr(link+0x10))if(const auto item=Qr(link))++proceed_kinds[static_cast<uint32_t>(Qr(Qr(item)+8)-e.image)];}
                for(auto at=first;at<end;at+=0x10)if(const auto item=Qr(at))++update_kinds[static_cast<uint32_t>(Qr(Qr(item)+8)-e.image)];}
            // I-0092: the particle items by DS3's ranges on the pool (296FF0's ranged GXFfxUpdateJob), in place of list order on this
            // thread; each reads its own node, and its random generator is the thread's own (1ACADB0).
            // Review M2: the generator's thread slot (1ACB000 allocates its TLS index at the first use, unsynchronised) taken once on
            // this thread before any range runs on the pool.
            static std::once_flag generator_slot;if(e.pool)std::call_once(generator_slot,[]{Call<uintptr_t>(0x1ACADB0);});
            std::atomic<uint64_t> run{};
            RunRanges(e.pool,(end-first)>>4,[&](size_t i){
                const auto at=first+0x10*i;const auto item=Qr(at),data=Qr(at+8);if(!item)return;const auto slot=Qr(Qr(item)+8);
                if(slot==particle){
                    reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint64_t)>(slot)(item,update+0x2270,data,0);
                    run.fetch_add(1,std::memory_order_relaxed);}});
            // I-0113 tranche 12 (gap 12): then the messages queued to effects during the manager's update (141ADAC30 to the effect's
            // node and each chained node with live particles, allocated by the pass's 2975E0), in list order on this thread: the item
            // is the node's receiver (node+0x90), its slot 141ADC190 hands the message to the node's renderer +0x58. An FFX model
            // animator's bone matrices (message 1000, E6B9D0 through 257660) pose the model particle there (1402823E0 writes the pose
            // without a lock: two nodes of one effect would race on the pool); the other renderers' +0x58 answer false (140267B20).
            // Without them Flame Fan's s08280 stayed at its bind pose, folded. A few dozen a minute.
            for(auto at=first;at<end;at+=0x10){const auto item=Qr(at);if(!item || Qr(Qr(item)+8)!=message)continue;
                reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint64_t)>(message)(item,update+0x2270,Qr(at+8),0);
                run.fetch_add(1,std::memory_order_relaxed);}
            update_stats.cpu_items_run+=run.load();}
        Call<void>(0x297410,combined);
        Call<void>(0x2969F0,proceed);
        Call<void>(0x296F40,update);
    }
    Call<void>(0x266DE0,sc+0x110,dt);
    B(sc+0x290,0);
    ++update_stats.updates;
}
UpdateStats Updates() noexcept {return update_stats;}
std::string ItemKinds() {
    const std::lock_guard hold(kinds_lock);std::string out="{\"proceed\":{";
    const auto list=[&out](std::map<uint32_t,uint64_t>& kinds){bool first=true;
        for(const auto& [rva,n]:kinds){char cell[48]{};sprintf_s(cell,"%s\"%X\":%llu",first?"":",",rva,static_cast<unsigned long long>(n));out+=cell;first=false;}
        kinds.clear();};
    list(proceed_kinds);out+="},\"update\":{";list(update_kinds);return out+"}}";}
}
