// FFX-DRAW N5b, I-0104: see effect_draw.h. The wrapper signatures are the decompiled originals' (FUN_1419202xx-1921xxx: the inner GX
// context, its +0x10, then the call's own arguments); their D3D11 slot in the comment is the method the original calls.
#include <intrin.h>
#include "effect_draw.h"
#include "effect_system.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ds3native::effect_draw {
namespace {
Engine engine{};
// Traps: the vtable of every token and of the inner context's +0 object. A call is a GX statement nobody captured: counted by slot.
std::array<std::atomic<uint64_t>,128> trap_calls{};
// I-0105 (Lightning Spear's bolt missing, i105-spells-2: 54,701 calls of slots 8 and 9): the 1st, 100th and 10,000th call of a slot
// name its caller (the return address, an image RVA) and the receiver's kind (a token's magic, else 0).
template<size_t Slot> uintptr_t __fastcall Trap(uintptr_t receiver) noexcept {
    const auto n=trap_calls[Slot].fetch_add(1)+1;
    if((n==1 || n==100 || n==10000) && engine.log){const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress());uint32_t magic{};
        if(receiver && engine.read)engine.read(receiver+0x10,&magic,4);
        char row[200]{};sprintf_s(row,"{\"event\":\"native_ffx_trap\",\"slot\":%zu,\"call\":%llu,\"caller\":\"%llX\",\"magic\":\"%08X\"}\n",Slot,static_cast<unsigned long long>(n),
            static_cast<unsigned long long>(caller>engine.image?caller-engine.image:caller),magic);engine.log(row);}
    return 0;}
template<size_t... I> constexpr std::array<uintptr_t,sizeof...(I)> TrapTable(std::index_sequence<I...>) {return {reinterpret_cast<uintptr_t>(&Trap<I>)...};}
const std::array<uintptr_t,128> trap_vtable=TrapTable(std::make_index_sequence<128>{});
uintptr_t Traps() noexcept {return reinterpret_cast<uintptr_t>(trap_vtable.data());}

std::mutex token_lock;   // the token stores below
std::unordered_map<uintptr_t,std::unique_ptr<Token>> texture_tokens;std::unordered_set<uintptr_t> token_addresses;
Token* Remember(Token* token) {token_addresses.insert(reinterpret_cast<uintptr_t>(token));return token;}
// The state bank (0x60 slots of 8 bytes) and its tokens, the default texture provider, the ring buffer token.
alignas(16) std::array<uintptr_t,0x60> bank{};std::array<Token,0x60> state_tokens{};Token default_token{},ring_token{},copy_token{},depth_token{};
// The frame copy and depth GX texture objects of the pass context: +0x48 their view (the token), the rest zero.
alignas(16) std::array<uintptr_t,16> copy_texture{},depth_texture{};
std::array<uint8_t,0x2000> device_shadow{};   // E07's settings shadow (Execute, DeviceShadow)
struct Provider {uintptr_t object{};} provider{};
struct ProviderObject {uintptr_t vtable{};} provider_object{};std::array<uintptr_t,8> provider_vtable{};
uintptr_t __fastcall DefaultTexture(uintptr_t) noexcept {return reinterpret_cast<uintptr_t>(&default_token);}
// The frame copy and depth texture objects answer as a GXTexture2D does at vt+0x10 (2AA60: the view at +0x48); the rest traps.
uintptr_t __fastcall TextureView(uintptr_t object) noexcept {return *reinterpret_cast<const uintptr_t*>(object+0x48);}
// I-0105 (the spell runs' traps, i105-spells-3: 2A6A30 at 2A6FEB/2A7000): the GPU-particle simulation asks the pass context's depth
// texture S+0x24D8 (and S+0x24E0) its width and height (a GXTexture2D's vt+0x40 / +0x48, 2AA80 / 2AAD0: the texture's description,
// 1924730) and stores them at S+0x300 / +0x304 (the object at S+0x24E0: +0x308 / +0x30C); natively they trapped (0, 0). The scene's size, the depth's and the copy's (Execute).
uint32_t __fastcall TextureWidth(uintptr_t,uint32_t) noexcept;uint32_t __fastcall TextureHeight(uintptr_t,uint32_t) noexcept;
std::array<uintptr_t,24> texture_object_vtable{};
// I-0140 tranche 10 (STUDY-LIGHTSHAFT-10003): the render targets (effect_draw.h). Each half-size target is a GX render target object
// of 24F2C0's shape (vtable 143D1D638, 0xA0 bytes): +0 a vtable (vt+0x20 its resource, vt+0x40 / +0x48 its width and height at a
// mip, the other slots traps), +0x48 its shader view, +0x70 its render-target view's provider (an object whose vt+8 returns the
// view). A view names its resource at +8: SetRenderTargetsWrapper (5F00) reads the kind and description through [view+8]+0x18
// (19281C0, 1924730): the resource's +0x18 null (no D3D11 object) and +0x20 a description record whose +8 is the
// D3D11_TEXTURE2D_DESC words and +0x18 the kind (3, a 2D texture). The scene's view (index 0) has no object.
struct TargetRecord {uintptr_t pad{};const uint32_t* words{};uintptr_t pad2{};uint32_t kind{3},pad3{};};
struct TargetResource {std::array<uintptr_t,3> pad{};uintptr_t d3d{};const TargetRecord* record{};};
struct TargetObject {uintptr_t vtable{};std::array<uintptr_t,8> fill1{};uintptr_t srv{};std::array<uintptr_t,4> fill2{};uintptr_t provider{};std::array<uintptr_t,5> fill3{};};
static_assert(offsetof(TargetObject,srv)==0x48 && offsetof(TargetObject,provider)==0x70 && sizeof(TargetObject)==0xA0);
static_assert(offsetof(TargetRecord,words)==8 && offsetof(TargetRecord,kind)==0x18 && offsetof(TargetResource,d3d)==0x18 && offsetof(TargetResource,record)==0x20);
std::array<TargetObject,effect_targets> target_objects{};std::array<std::array<Token,3>,effect_targets> target_tokens{};
std::array<TargetResource,effect_targets> target_resources{};std::array<TargetRecord,effect_targets> target_records{};
std::array<std::array<uint32_t,12>,effect_targets> target_words{};
std::array<uintptr_t,24> target_vtable{},target_provider_vtable{};   // the final review's L5: as wide as texture_object_vtable
uint32_t TargetObjectIndex(uintptr_t object) noexcept {   // 1 or 2 for a half-size target's object, else 0
    const auto first=reinterpret_cast<uintptr_t>(target_objects.data());
    if(object<first+sizeof(TargetObject) || object>=first+sizeof(target_objects))return 0;
    return static_cast<uint32_t>((object-first)/sizeof(TargetObject));}
uintptr_t __fastcall TargetResourceOf(uintptr_t object) noexcept {
    const auto i=TargetObjectIndex(object);return i?reinterpret_cast<uintptr_t>(&target_tokens[i][size_t(TargetRole::Resource)]):0;}
uint32_t __fastcall TargetWidth(uintptr_t object,uint32_t mip) noexcept {const auto i=TargetObjectIndex(object);return i?std::max(1u,target_words[i][0]>>(mip&31)):0;}
uint32_t __fastcall TargetHeight(uintptr_t object,uint32_t mip) noexcept {const auto i=TargetObjectIndex(object);return i?std::max(1u,target_words[i][1]>>(mip&31)):0;}
uintptr_t __fastcall TargetRtv(uintptr_t holder,uint32_t) noexcept {   // holder: the object's +0x70
    const auto i=TargetObjectIndex(holder-offsetof(TargetObject,provider));return i?reinterpret_cast<uintptr_t>(&target_tokens[i][size_t(TargetRole::Rtv)]):0;}
std::once_flag statics_once;
void Statics() {
    std::call_once(statics_once,[]{
        const std::lock_guard hold(token_lock);
        for(size_t i=0;i<bank.size();++i){auto& t=state_tokens[i];t.vtable=Traps();t.magic=state_magic;t.value=static_cast<uint32_t>(i*8);
            bank[i]=reinterpret_cast<uintptr_t>(Remember(&t));}
        default_token.vtable=Traps();default_token.magic=default_magic;Remember(&default_token);
        ring_token.vtable=Traps();ring_token.magic=ring_magic;Remember(&ring_token);
        copy_token.vtable=Traps();copy_token.magic=copy_magic;Remember(&copy_token);depth_token.vtable=Traps();depth_token.magic=depth_magic;Remember(&depth_token);
        std::copy_n(trap_vtable.begin(),texture_object_vtable.size(),texture_object_vtable.begin());texture_object_vtable[2]=reinterpret_cast<uintptr_t>(&TextureView);   // the trap functions, not the table
        texture_object_vtable[8]=reinterpret_cast<uintptr_t>(&TextureWidth);texture_object_vtable[9]=reinterpret_cast<uintptr_t>(&TextureHeight);
        copy_texture[0]=depth_texture[0]=reinterpret_cast<uintptr_t>(texture_object_vtable.data());
        copy_texture[9]=reinterpret_cast<uintptr_t>(&copy_token);depth_texture[9]=reinterpret_cast<uintptr_t>(&depth_token);
        std::copy_n(trap_vtable.begin(),provider_vtable.size(),provider_vtable.begin());provider_vtable[2]=reinterpret_cast<uintptr_t>(&DefaultTexture);
        provider_object.vtable=reinterpret_cast<uintptr_t>(provider_vtable.data());provider.object=reinterpret_cast<uintptr_t>(&provider_object);
        // I-0140 tranche 10: the targets' tokens, resources, records and objects (their description words are written by Execute).
        std::copy_n(trap_vtable.begin(),target_vtable.size(),target_vtable.begin());target_vtable[4]=reinterpret_cast<uintptr_t>(&TargetResourceOf);
        target_vtable[8]=reinterpret_cast<uintptr_t>(&TargetWidth);target_vtable[9]=reinterpret_cast<uintptr_t>(&TargetHeight);
        std::copy_n(trap_vtable.begin(),target_provider_vtable.size(),target_provider_vtable.begin());target_provider_vtable[1]=reinterpret_cast<uintptr_t>(&TargetRtv);
        for(uint32_t i=0;i<effect_targets;++i){
            target_records[i].words=target_words[i].data();target_resources[i].record=&target_records[i];
            for(const auto role:{TargetRole::Rtv,TargetRole::Srv,TargetRole::Resource}){auto& t=target_tokens[i][size_t(role)];
                t.vtable=Traps();t.magic=target_magic;t.value=TargetValue(i,role);Remember(&t);}
            target_tokens[i][size_t(TargetRole::Rtv)].none=reinterpret_cast<uintptr_t>(&target_resources[i]);   // [view+8], read by 5F00
            if(i){auto& o=target_objects[i];o.vtable=reinterpret_cast<uintptr_t>(target_vtable.data());
                o.srv=reinterpret_cast<uintptr_t>(&target_tokens[i][size_t(TargetRole::Srv)]);o.provider=reinterpret_cast<uintptr_t>(target_provider_vtable.data());}}});
}

uint32_t __fastcall TextureWidth(uintptr_t,uint32_t mip) noexcept {return engine.width>>(mip&31);}   // 2AA80: the description's width >> mip
uint32_t __fastcall TextureHeight(uintptr_t,uint32_t mip) noexcept {return engine.height>>(mip&31);}   // 2AAD0

// The native GX contexts: readable, writable state caches (the original's fields up to +0x2200 are written by slot 27 and the
// builders), the inner's +0 the trap object, the outer's +8 the inner. The selector (+0x458) is only compared.
alignas(16) std::array<uint8_t,0x2400> outer{},inner{};
struct TrapObject {uintptr_t vtable{};} trap_object{};
struct Selector {uintptr_t object{};} selector{};
uintptr_t Inner() noexcept {return reinterpret_cast<uintptr_t>(inner.data());}

// The arena (I-0104): the ring's blocks and the constant blocks of a frame, reset each frame; its first zero_bytes stay zero. Its
// capacity never changes during a frame (the builders hold the ring's pointers between map and unmap); after a frame that ran out
// of it, it is doubled at the next Execute's start (review: no pointer is held across frames), up to arena_limit. The ring: holder →
// ring object → vtable {trap, map, unmap}.
constexpr size_t arena_bytes=0x400000,arena_limit=0x4000000;   // the original's ring budget is 2 MB a frame (P5); the constant blocks share the arena
std::atomic<int> patch_rows_left{};   // RequestPatchRows
std::atomic<int> vs_rows_left{};      // RequestVsRows
std::vector<uint8_t> arena(arena_bytes);size_t arena_used{zero_bytes},block_start{},arena_peak{};uint64_t ring_refusals{},arena_refusals{};
bool arena_full{};   // a reservation was refused this frame
uint32_t Reserve(size_t size,size_t alignment) noexcept {
    const size_t at=(arena_used+alignment-1)&~(alignment-1);if(at+size>arena.size()){arena_full=true;return UINT32_MAX;}
    arena_used=at+size;arena_peak=std::max(arena_peak,arena_used);return static_cast<uint32_t>(at);}
struct RingObject {uintptr_t vtable{};} ring_object{};struct RingHolder {uintptr_t object{};} ring_holder{};std::array<uintptr_t,8> ring_vtable{};
int32_t __fastcall RingMap(uintptr_t,uintptr_t* out,uintptr_t,uint32_t bytes) noexcept {
    const auto at=out?Reserve(bytes,16):UINT32_MAX;
    if(at==UINT32_MAX){if(out)*out=0;++ring_refusals;return 1;}   // the builders check the result (385F70 leaves)
    block_start=at;*out=reinterpret_cast<uintptr_t>(arena.data()+at);return 0;}
void __fastcall RingUnmap(uintptr_t,uintptr_t* buffer,uint32_t* offset,uintptr_t) noexcept {
    if(buffer)*buffer=reinterpret_cast<uintptr_t>(&ring_token);if(offset)*offset=static_cast<uint32_t>(block_start);}

// The context's state, and the caller's emit (Execute).
EffectDrawRecord pending{};int32_t current_pass{};uint32_t current_key{};uintptr_t current_class{};   // current_class: the running renderer's vtable RVA (witness)
StateOrigin blend_origin{},depth_origin{};   // I-0113 gap 5: the command that bound pending.blend and pending.depth (ModelSubmission)
StateOrigin skipped_origin{};   // I-0113 gap 5 (step review L1): the last command not run natively, which may bind states in the original
std::array<CommandRun,16> command_ring{};uint64_t command_count{};   // witness only: the commands run in this execution (ModelSubmission::sequence)
const std::function<void(const EffectDrawRecord&)>* emit_{};
// I-0140 tranche 10: the light shafts' class (271B40's renderer) and the statements its running command emitted (native_ffx_light_shaft).
constexpr uintptr_t light_shaft_class=0x3D65A88;
struct ShaftCommand {uint32_t draws{},clears{},copies{},other{};} shaft_command{};uint64_t shaft_commands{},shaft_owner_skips{};
std::exception_ptr emit_failure;uint64_t emit_failures{};
// Witnesses (ffx-record.flag only, I-0104: no string work in play): wrapper calls by kind, bound objects by slot and kind, draws by pass
// and topology.
std::map<std::string,uint64_t> calls;std::map<std::pair<int32_t,uint32_t>,uint64_t> draws;std::map<std::string,uint64_t> bound;
uint64_t frames{},commands_run{};
bool Ours(uintptr_t context) noexcept {return context==Inner();}
void Call(const char* name) {if(engine.witness)++calls[name];}
std::string Kind(uintptr_t object) {
    if(!object)return "null";
    if(const auto* t=AsToken(object))return t->magic==texture_magic?"ffx_texture":t->magic==state_magic?"state":t->magic==default_magic?"default":t->magic==program_magic?"program":t->magic==copy_magic?"copy":t->magic==depth_magic?"depth":"ring";
    uint32_t kind{};std::array<uint32_t,8> d{};if(effect_system::Describe(object,kind,d))return "effect_object_"+std::to_string(kind);
    return "engine";}
void Bound(const char* slot,uintptr_t object) {if(engine.witness)++bound[std::string(slot)+":"+Kind(object)];}
// A bind of `count` objects from `start`: the slots it names are replaced (GX state is per slot).
void Note(uintptr_t object);
void Bind(std::array<uintptr_t,16>& slots,const char* name,uint32_t start,uint32_t count,uintptr_t array) {
    for(uint32_t i=0;i<count && start+i<slots.size();++i){uintptr_t object{};if(array)engine.read(array+8*i,&object,8);slots[start+i]=object;Bound(name,object);
        if(name[0]=='c' || name[0]=='g' || !std::strcmp(name,"vs_srv"))Note(object);}
    Call(name);}
// The bytes of each effect constant buffer's last UpdateSubresource (the buffers persist across frames, as the original's), and this
// frame's copy of them in the arena (I-0104: placed once per update, at the first statement that binds the buffer).
std::unordered_map<uintptr_t,std::vector<uint8_t>> constant_bytes;std::unordered_set<uintptr_t> invalid_constants;   // last update partial or unreadable
std::unordered_map<uintptr_t,Range> placed;
Range Place(uintptr_t buffer,bool& invalid) {
    if(!buffer)return {};
    if(invalid_constants.count(buffer)){invalid=true;return {};}
    if(const auto found=placed.find(buffer);found!=placed.end())return found->second;
    std::vector<uint8_t> created;const std::vector<uint8_t>* bytes=&created;
    if(const auto found=constant_bytes.find(buffer);found!=constant_bytes.end())bytes=&found->second;
    else effect_system::BufferBytes(buffer,created);   // never updated: its creation bytes (none: zero, as created)
    size_t size=bytes->size();
    if(!size){uint32_t kind{};std::array<uint32_t,8> d{};if(effect_system::Describe(buffer,kind,d) && kind==1 && d[0]>zero_bytes)size=d[0];}   // larger than the zero block
    Range range{};
    if(size){const size_t padded=(size+255)&~size_t{255};const auto at=Reserve(padded,256);
        if(at==UINT32_MAX){invalid=true;++arena_refusals;return {};}
        if(!bytes->empty())std::memcpy(arena.data()+at,bytes->data(),bytes->size());
        std::memset(arena.data()+at+bytes->size(),0,padded-bytes->size());range={at,static_cast<uint32_t>(padded)};}
    placed.emplace(buffer,range);return range;}

// The wrappers' originals (set by the bootstrap's hooks) and detours.
using W2=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t);
using WTopology=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t);
using WIndex=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uint32_t);
using WVertex=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uintptr_t,uintptr_t,uintptr_t);
using WArray=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uintptr_t);
using WUpdate=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uintptr_t,uintptr_t,uint32_t,uint32_t);
using WBlend=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t,uint32_t);
using WDepth=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint8_t);
using WDraw=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t);
using WDrawIndexed=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,int32_t);
using WType=uint64_t(__fastcall*)(uintptr_t);
W2 layout_original{},vs_original{},hs_original{},ds_original{},gs_original{},raster_original{},ps_original{};
WTopology topology_original{};WIndex index_original{};WVertex vertex_original{};
WArray vs_cb_original{},ds_cb_original{},ps_cb_original{},ps_srv_original{},ps_sampler_original{};
WUpdate update_original{};WBlend blend_original{};WDepth depth_original{};WDraw draw_original{};WDrawIndexed indexed_original{};WType type_original{};

void __fastcall Update(uintptr_t c,uintptr_t s,uintptr_t dst,uint32_t sub,uintptr_t box,uintptr_t src,uint32_t pitch,uint32_t slice) noexcept {   // vt+0x180 UpdateSubresource
    if(!Ours(c)){update_original(c,s,dst,sub,box,src,pitch,slice);return;}
    try{   // the hook is noexcept (step review L4): a failed copy leaves the buffer invalid, its draws dropped
    Call("update");uint32_t kind{};std::array<uint32_t,8> d{};placed.erase(dst);
    if(effect_system::Describe(dst,kind,d) && kind==1 && src && !box && d[0] && d[0]<=0x10000){auto& bytes=constant_bytes[dst];bytes.resize(d[0]);if(!engine.read(src,bytes.data(),bytes.size())){bytes.clear();invalid_constants.insert(dst);}else invalid_constants.erase(dst);
        // Snow witness (ffx-record.flag): a VSCommonBuffer image (g_near at +0x14C in (0, 1), g_far at +0x148 > 1000): its +0x120..+0x13F
        // (fog, g_screenSize, g_recScreenSize) and the writer (the wrapper's caller), first per caller then every 5000th.
        if(engine.witness && engine.log && bytes.size()>=0x150){float near_plane{},far_plane{};std::memcpy(&near_plane,bytes.data()+0x14C,4);std::memcpy(&far_plane,bytes.data()+0x148,4);
            if(near_plane>0 && near_plane<1 && far_plane>1000){static std::unordered_map<uintptr_t,uint64_t> writers;const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-engine.image;
                if(vs_rows_left.load()>0 && vs_rows_left.fetch_sub(1)>0){std::string vs="{\"event\":\"ffx_vs_image\",\"mode\":\"native\",\"caller\":\"";char w[24]{};sprintf_s(w,"%llX\",\"f\":[",static_cast<unsigned long long>(caller));vs+=w;
                    for(size_t k=0;k<0x150/4;++k){float f{};std::memcpy(&f,bytes.data()+4*k,4);if(std::isfinite(f))sprintf_s(w,"%s%.6g",k?",":"",f);else sprintf_s(w,"%snull",k?",":"");vs+=w;}
                    vs+="]}\n";engine.log(vs.c_str());}
                if(const auto n=++writers[caller];n==1 || n%5000==0){std::array<float,8> f{};std::memcpy(f.data(),bytes.data()+0x120,32);char row[300]{};
                    sprintf_s(row,"{\"event\":\"ffx_vs_common\",\"mode\":\"native\",\"caller\":\"%llX\",\"n\":%llu,\"bytes\":%zu,\"w120\":[%g,%g,%g,%g,%g,%g,%g,%g]}\n",static_cast<unsigned long long>(caller),
                        static_cast<unsigned long long>(n),bytes.size(),f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7]);engine.log(row);}}}
        // I-0140 (STUDY-NOT-YET-NATIVE item 8, « GPU particles read a zero SH »): 2A7E70 writes its cbCubeEnvSH1 itself (48 bytes: the
        // system's SH S+0x2A20..+0x2A4C times the constants 1439F24B0 and its light's colour [[S+0x2B80]+8]+0x110..+0x118) through this
        // wrapper into the settings shadow's buffer; the first 8 of those images and every 600th, so the witness tells zero from read.
        if(engine.witness && engine.log && bytes.size()==48){const auto caller=reinterpret_cast<uintptr_t>(_ReturnAddress())-engine.image;
            if(caller>=0x2A7E70 && caller<0x2A9000){static uint64_t images{};if(const auto n=++images;n<=8 || n%600==0){std::array<float,12> f{};std::memcpy(f.data(),bytes.data(),48);
                char row[400]{};sprintf_s(row,"{\"event\":\"native_gpu_particle_sh\",\"n\":%llu,\"caller\":\"%llX\",\"sh\":[%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g,%g]}\n",
                    static_cast<unsigned long long>(n),static_cast<unsigned long long>(caller),f[0],f[1],f[2],f[3],f[4],f[5],f[6],f[7],f[8],f[9],f[10],f[11]);engine.log(row);}}}}
    else{Call("update_unowned");constant_bytes.erase(dst);invalid_constants.insert(dst);}   // review: a partial or foreign update leaves no stale bytes (the draw is dropped)
    }catch(...){constant_bytes.erase(dst);try{invalid_constants.insert(dst);}catch(...){}}}
void __fastcall Layout(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(!Ours(c)){layout_original(c,s,p);return;}Call("layout");Bound("layout",p);pending.layout=p;}             // vt+0x88
void __fastcall Topology(uintptr_t c,uintptr_t s,uint32_t t) noexcept {if(!Ours(c)){topology_original(c,s,t);return;}Call("topology");pending.topology=t;}   // vt+0xC0
void __fastcall IndexBuffer(uintptr_t c,uintptr_t s,uintptr_t b,uint32_t format,uint32_t offset) noexcept {                                                    // vt+0x98
    if(!Ours(c)){index_original(c,s,b,format,offset);return;}Call("index_buffer");Bound("ib",b);pending.index_buffer=b;pending.index_format=format;pending.index_offset=offset;}
void __fastcall VertexBuffers(uintptr_t c,uintptr_t s,uint32_t start,uint32_t count,uintptr_t buffers,uintptr_t strides,uintptr_t offsets) noexcept {          // vt+0x90
    if(!Ours(c)){vertex_original(c,s,start,count,buffers,strides,offsets);return;}
    Call("vertex_buffer");if(start || count!=1)Call("vertex_buffer_not_slot0");
    if(buffers)engine.read(buffers,&pending.vertex_buffer,8);if(strides)engine.read(strides,&pending.stride,4);if(offsets)engine.read(offsets,&pending.vertex_offset,4);Bound("vb",pending.vertex_buffer);}
void __fastcall VsConstants(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){vs_cb_original(c,s,a,n,p);return;}Bind(pending.vs_constants,"vs_cb",a,n,p);}  // vt+0x38
void __fastcall Vs(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(!Ours(c)){vs_original(c,s,p);return;}Call("vs");Bound("vs",p);pending.vs=p;}                            // vt+0x58
void __fastcall Hs(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(!Ours(c)){hs_original(c,s,p);return;}Call("hs");Bound("hs",p);pending.hs=p;}                            // vt+0x1E0
void __fastcall DsConstants(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){ds_cb_original(c,s,a,n,p);return;}Bind(pending.ds_constants,"ds_cb",a,n,p);}  // vt+0x210
void __fastcall Ds(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(!Ours(c)){ds_original(c,s,p);return;}Call("ds");Bound("ds",p);pending.ds=p;}                            // vt+0x200
void __fastcall Gs(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(!Ours(c)){gs_original(c,s,p);return;}Call("gs");Bound("gs",p);pending.gs=p;}                            // vt+0xB8
void __fastcall Raster(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(!Ours(c)){raster_original(c,s,p);return;}Call("raster");Bound("raster",p);pending.raster=p;}            // vt+0x158
void __fastcall PsSamplers(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){ps_sampler_original(c,s,a,n,p);return;}Bind(pending.ps_samplers,"ps_sampler",a,n,p);}  // vt+0x50
void __fastcall PsResources(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){ps_srv_original(c,s,a,n,p);return;}Bind(pending.ps_resources,"ps_srv",a,n,p);}   // vt+0x40
void __fastcall PsConstants(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){ps_cb_original(c,s,a,n,p);return;}Bind(pending.ps_constants,"ps_cb",a,n,p);}  // vt+0x80
void __fastcall Ps(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(!Ours(c)){ps_original(c,s,p);return;}Call("ps");Bound("ps",p);pending.ps=p;}                            // vt+0x48
void __fastcall Blend(uintptr_t c,uintptr_t s,uintptr_t p,uintptr_t factor,uint32_t mask) noexcept {if(!Ours(c)){blend_original(c,s,p,factor,mask);return;}Call("blend");Bound("blend",p);pending.blend=p;
    blend_origin={current_pass,current_key,current_class};if(engine.witness && command_count)++command_ring[(command_count-1)%command_ring.size()].blends;}  // vt+0x118
void __fastcall Depth(uintptr_t c,uintptr_t s,uintptr_t p,uint8_t ref) noexcept {if(!Ours(c)){depth_original(c,s,p,ref);return;}Call("depth");Bound("depth",p);pending.depth=p;depth_origin={current_pass,current_key,current_class};}  // vt+0x120
// E07 witness: each effect object the GPU-particle statements reach, once: its kind and description words, and for a view its
// buffer's (the D3D12 resources and views are to be made from these).
std::unordered_set<uintptr_t> noted;
std::unordered_set<uint64_t> skipped_told;   // I-0113 tranche 23: the skipped kinds already logged (Execute's thread only, the main collect, as `calls`)
void Note(uintptr_t object) {
    if(!engine.gpu_particles || !engine.witness || !object || !engine.log || !noted.insert(object).second)return;
    uint32_t kind{};std::array<uint32_t,8> d{};if(!effect_system::Describe(object,kind,d))return;
    const auto parent=effect_system::ViewParent(object);uint32_t parent_kind{};std::array<uint32_t,8> p{};if(parent)effect_system::Describe(parent,parent_kind,p);
    char row[400]{};sprintf_s(row,"{\"event\":\"native_ffx_gpu_object\",\"kind\":%u,\"description\":[%u,%u,%u,%u,%u,%u,%u,%u],\"buffer\":[%u,%u,%u,%u,%u,%u]}\n",kind,
        d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],p[0],p[1],p[2],p[3],p[4],p[5]);engine.log(row);}
// E07 (EFFECTS-E07-GPU-PARTICLES-NATIVE §3): the compute, copy, clear and indirect statements of the GPU-particle simulation
// (2A6A30) and draw (2A7E70), handed to the caller as the draws are (Emit).
using W3=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
using WUav=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uintptr_t,uintptr_t);
using WDispatch=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uint32_t,uint32_t);
using WIndirect=void(__fastcall*)(uintptr_t,uintptr_t);
W3 clear_rtv_original{},clear_uav_original{},copy_original{};W2 cs_original{};
WArray gs_cb_original{},vs_srv_original{},cs_srv_original{},cs_cb_original{};WUav cs_uav_original{};WDispatch dispatch_original{};
WIndirect dispatch_indirect_original{},draw_indirect_original{};
void Emit(Op op);
// I-0140 tranche 10: a target token of the given role, its index in `index`.
bool TargetOf(uintptr_t object,TargetRole role,uint8_t& index) noexcept {
    const auto* t=object?AsToken(object):nullptr;
    if(!t || t->magic!=target_magic || (t->value&3)!=static_cast<uint32_t>(role) || (t->value>>2)>=effect_targets)return false;
    index=static_cast<uint8_t>(t->value>>2);return true;}
void __fastcall ClearRtv(uintptr_t c,uintptr_t s,uintptr_t v,uintptr_t colour) noexcept {   // vt+0x190
    if(!Ours(c)){clear_rtv_original(c,s,v,colour);return;}Call("clear_rtv");
    // I-0140 tranche 10: a half-size target's clear (271B40 clears both before the shaft's mask); any other view is not ours.
    if(uint8_t i{};TargetOf(v,TargetRole::Rtv,i)){
        if(i){pending.clear_target=i;pending.clear_colour={};if(colour)engine.read(colour,pending.clear_colour.data(),16);Emit(Op::ClearTarget);}
        else Call("clear_rtv_scene");}   // the pass's own target: not cleared by an effect natively (none does so in the original's 271B40)
    else Call("clear_rtv_foreign");}
// I-0140 tranche 10: the render targets (1920F40, vt+0x108 OMSetRenderTargets: count, views, depth view), the viewports (1920CD0,
// vt+0x160) and the scissor rectangles (1920CF0, vt+0x170) the native context is given; the first of each is kept (the depth view
// is not: the stage keeps its own depth policy, the half-size targets having none).
using WTargets=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uintptr_t,uintptr_t);using WRects=void(__fastcall*)(uintptr_t,uintptr_t,uint32_t,uintptr_t);
WTargets targets_original{};WRects viewports_original{},scissors_original{};
void __fastcall RenderTargets(uintptr_t c,uintptr_t s,uint32_t count,uintptr_t views,uintptr_t depth) noexcept {
    if(!Ours(c)){targets_original(c,s,count,views,depth);return;}
    Call("render_targets");uintptr_t first{};if(count && views)engine.read(views,&first,8);
    if(uint8_t i{};TargetOf(first,TargetRole::Rtv,i)){pending.target=i;return;}
    pending.target=first?target_foreign:target_unbound;Call(first?"render_targets_foreign":"render_targets_unbound");
    // The final review's L2: a bind by another family that may make its later draws drop (no view, or one not ours) is told once a
    // class and kind in every run (no such bind is known: no trap row in the I-0139 and I-0140 journals shows a family reaching this
    // wrapper before the tranche; 271B40 unbinds only between its own draws). Execute's thread only (its one caller, 21_effects.inc); the
    // told table allocates nothing (the witness counters, Call with ffx-record.flag, do, as in every hook here).
    if(engine.log && current_class!=light_shaft_class){static std::array<uint64_t,16> told{};static size_t told_count{};
        const uint64_t key=uint64_t(current_class)<<1|(first?1u:0u);
        if(told_count<told.size() && std::find(told.begin(),told.begin()+ptrdiff_t(told_count),key)==told.begin()+ptrdiff_t(told_count)){told[told_count++]=key;char row[160]{};
            sprintf_s(row,"{\"event\":\"native_ffx_render_target_drop\",\"class\":\"%llX\",\"kind\":\"%s\"}\n",static_cast<unsigned long long>(current_class),first?"foreign":"unbound");engine.log(row);}}}
void __fastcall Viewports(uintptr_t c,uintptr_t s,uint32_t count,uintptr_t rects) noexcept {
    if(!Ours(c)){viewports_original(c,s,count,rects);return;}
    Call("viewports");pending.viewport_set=count && rects && engine.read(rects,pending.viewport.data(),sizeof(pending.viewport));}
void __fastcall Scissors(uintptr_t c,uintptr_t s,uint32_t count,uintptr_t rects) noexcept {
    if(!Ours(c)){scissors_original(c,s,count,rects);return;}
    Call("scissors");pending.scissor_set=count && rects && engine.read(rects,pending.scissor.data(),sizeof(pending.scissor));}
void __fastcall ClearUav(uintptr_t c,uintptr_t s,uintptr_t v,uintptr_t values) noexcept {   // vt+0x198
    if(!Ours(c)){clear_uav_original(c,s,v,values);return;}Call("clear_uav");Bound("clear_uav",v);Note(v);
    pending.clear_view=v;pending.clear_values={};if(values)engine.read(values,pending.clear_values.data(),16);Emit(Op::ClearUav);}
void __fastcall Copy(uintptr_t c,uintptr_t s,uintptr_t dst,uintptr_t src) noexcept {   // vt+0x178
    if(!Ours(c)){copy_original(c,s,dst,src);return;}Call("copy");
    // I-0140 tranche 10: a whole copy between two targets (271B40's ping-pong, the resources its objects' vt+0x20 return).
    {uint8_t d{},r{};const bool to_target=TargetOf(dst,TargetRole::Resource,d),from_target=TargetOf(src,TargetRole::Resource,r);
        if(to_target || from_target){   // the half-size targets only (the scene's resource is not handed out)
            if(to_target && from_target && d && r && d!=r){pending.copy_target_destination=d;pending.copy_target_source=r;Emit(Op::CopyTarget);}
            else Call("copy_target_foreign");
            return;}}
    Bound("copy_dst",dst);Bound("copy_src",src);Note(dst);Note(src);
    pending.destination=dst;pending.source=src;Emit(Op::Copy);}
// I-0113 tranche 23 (gap 22): the draw context's copy slot 140260440(dc, rect), which the distortion quads call before their draw: the
// original copies the rectangle rect (left, top, right, bottom in [0, 1] of the bound target, the target's [dc+0x20]+0x4F8) widened by
// 16 pixels from the target into the frame copy ([dc+0xA70]+8) at the same place, through 1920280 (CopySubresourceRegion), when both
// textures have the same format; the box (0, 0) to the view array's size when [dc+0xA88] holds more than one view (CopyBranch). Natively the target is not a
// GX texture (the slot copied nothing): the statement goes to the renderer in its place, before the quad. The draw context is ours
// when its GX context [[dc+0x100]+8] is. The draw context's slot 20 (262A70, the trails' appearance setup, 606 and 10012) makes the
// same copy over the whole target (its rectangle 0..size, XMM6 zero at 140262F63; the same +0xA88 branch) when the FXR property it
// evaluates from the resource's +0x280 is above 0 (stored at dc+0x5A0): hooked after the original (study thread S, 2026-11-07).
bool OursDrawContext(uintptr_t dc) noexcept {   // the step review's L4: no read before the first Execute set the reader
    uintptr_t gx_outer{},gx_inner{};return dc && engine.read && engine.read(dc+0x100,&gx_outer,8) && gx_outer && engine.read(gx_outer+8,&gx_inner,8) && Ours(gx_inner);}
bool HasFrameCopy(uintptr_t dc) noexcept {uintptr_t frame_copy{};return engine.read(dc+0xA70,&frame_copy,8) && frame_copy;}   // the original copies nothing without one
// The +0xA88 branch (both slots): a view array with more than one view (+0x70) gives the box (0, 0) to [+0x7C] + 2 x [+0x78],
// [+0x80] + 2 x [+0x78] (the step review's L1: read, not assumed to be the whole target).
void CopyBranch(uintptr_t dc) noexcept {
    uintptr_t views{};uint32_t count{};std::array<int32_t,3> w{};
    if(engine.read(dc+0xA88,&views,8) && views && engine.read(views+0x70,&count,4) && count>1 && engine.read(views+0x78,w.data(),12)){
        const auto size=[&](int32_t v){const int64_t n=int64_t(v)+2*int64_t(w[0]);return int32_t(n<0?0:n>INT32_MAX?INT32_MAX:n);};   // L5: game words, no overflow
        pending.copy_kind=2;pending.copy_size={size(w[1]),size(w[2])};}}
using WCopyRect=void(__fastcall*)(uintptr_t,const float*);WCopyRect copy_rect_original{};
void __fastcall CopyRect(uintptr_t dc,const float* rect) noexcept {
    if(!rect || !OursDrawContext(dc)){copy_rect_original(dc,rect);return;}
    Call("copy_rect");if(!HasFrameCopy(dc) || !engine.read(reinterpret_cast<uintptr_t>(rect),pending.copy_rect.data(),16))return;
    pending.copy_kind=0;CopyBranch(dc);Emit(Op::CopyRect);}
using WAppearance=void(__fastcall*)(uintptr_t,uint32_t,uintptr_t,uintptr_t);WAppearance appearance_original{};
void __fastcall TrailAppearance(uintptr_t dc,uint32_t kind,uintptr_t resource,uintptr_t particle) noexcept {
    appearance_original(dc,kind,resource,particle);
    // The original's gates before +0x5A0 is written (the final review's L3): [dc+0xA60] set, the resource's +0x14 above 0x1B and +0x1C above 6.
    uintptr_t holder{};uint32_t counts[3]{};
    if(!OursDrawContext(dc) || !engine.read(dc+0xA60,&holder,8) || !holder || !resource || !engine.read(resource+0x14,counts,12) || counts[0]<=0x1B || counts[2]<=6)return;
    float property{};if(!engine.read(dc+0x5A0,&property,4) || !(property>0) || !HasFrameCopy(dc))return;
    Call("copy_rect_trail");pending.copy_kind=1;CopyBranch(dc);Emit(Op::CopyRect);}
void __fastcall GsConstants(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){gs_cb_original(c,s,a,n,p);return;}Bind(pending.gs_constants,"gs_cb",a,n,p);}   // vt+0xB0
void __fastcall VsResources(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){vs_srv_original(c,s,a,n,p);return;}Bind(pending.vs_resources,"vs_srv",a,n,p);}   // vt+0xC8
void __fastcall CsResources(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){cs_srv_original(c,s,a,n,p);return;}Bind(pending.cs_resources,"cs_srv",a,n,p);}   // vt+0x218
void __fastcall CsUavs(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p,uintptr_t counts) noexcept {   // vt+0x220
    // The initial counts (-1 keeps the hidden counter): the particle programmes append and consume (EFFECTS-E07 3.1: 0 on the first
    // emission, 0 for the update's append buffer and the counter), so the counter each view carries natively takes them.
    if(!Ours(c)){cs_uav_original(c,s,a,n,p,counts);return;}Bind(pending.cs_uavs,"cs_uav",a,n,p);
    for(uint32_t i=0;i<n && a+i<16;++i){uint32_t v=UINT32_MAX;if(counts)engine.read(counts+4*i,&v,4);const uint16_t bit=uint16_t(1u<<(a+i));
        if(v!=UINT32_MAX){pending.cs_uav_counts[a+i]=v;pending.cs_uav_counts_set|=bit;}else pending.cs_uav_counts_set&=uint16_t(~bit);}}
void __fastcall Cs(uintptr_t c,uintptr_t s,uintptr_t p) noexcept {if(!Ours(c)){cs_original(c,s,p);return;}Call("cs");Bound("cs",p);pending.cs=p;}   // vt+0x228
void __fastcall CsConstants(uintptr_t c,uintptr_t s,uint32_t a,uint32_t n,uintptr_t p) noexcept {if(!Ours(c)){cs_cb_original(c,s,a,n,p);return;}Bind(pending.cs_constants,"cs_cb",a,n,p);}   // vt+0x238
void __fastcall Dispatch(uintptr_t c,uintptr_t s,uint32_t x,uint32_t y,uint32_t z) noexcept {   // vt+0x148
    if(!Ours(c)){dispatch_original(c,s,x,y,z);return;}Call("dispatch");pending.groups[0]=x;pending.groups[1]=y;pending.groups[2]=z;Emit(Op::Dispatch);}
// 1400C5F40 / 1400C61B0 take the outer context (they call [inner]'s vt+0x150 / +0x140 themselves).
bool OursOuter(uintptr_t outer_context) noexcept {return outer_context==reinterpret_cast<uintptr_t>(outer.data());}
// I-0140 tranche 10 (the first witness, i140-e30: 0xC0000005 in 1400C4F10+0xEC under 271B40 in 30000010 and 38000020): 271B40 first
// builds a GX state block on its outer context through C4F10, which copies its constants and reads the 2D virtual extent
// [[144961AB0]+0x88]+0xF8/+0xFC (the GX display record, null natively). On the native outer context its body is made here, the
// original's statement for statement (READ, its disassembly), the extent taken as the scene's size (the original's is the window's,
// 1280x720 in a 1280x720 window, LEGACY-ROOT-INVENTORY-903); 271B40 overwrites the block before it reads any of it (271B40.c: its
// first 0x140 bytes zeroed and refilled before any read). Any other context runs the original.
using WStateBlock=uintptr_t(__fastcall*)(uintptr_t,uintptr_t);WStateBlock state_block_original{};
uintptr_t __fastcall StateBlock(uintptr_t block,uintptr_t outer_context) noexcept {
    if(!OursOuter(outer_context) || !engine.read)return state_block_original(block,outer_context);
    Call("state_block");
    const auto put=[&](uint32_t at,const void* from,size_t n){std::memcpy(reinterpret_cast<void*>(block+at),from,n);};
    std::array<uint8_t,16> a{},b{},c3{},d{},e{};float f{};
    engine.read(engine.image+0x3D1BFA0,a.data(),16);engine.read(engine.image+0x3D1BFB0,b.data(),16);engine.read(engine.image+0x3D1BF90,c3.data(),16);
    engine.read(engine.image+0x3D1C080,d.data(),16);engine.read(engine.image+0x39F2490,e.data(),16);engine.read(engine.image+0x473F6D8,&f,4);
    uintptr_t inner_context{};std::memcpy(&inner_context,reinterpret_cast<const void*>(outer_context+8),8);
    put(0,&outer_context,8);put(8,&inner_context,8);
    put(0xE0,a.data(),16);put(0xF0,b.data(),16);put(0x100,d.data(),16);put(0xD0,c3.data(),16);put(0x90,c3.data(),16);
    put(0xA0,a.data(),16);put(0xB0,b.data(),16);put(0xC0,d.data(),16);put(0x10,c3.data(),16);put(0x20,a.data(),16);put(0x30,b.data(),16);put(0x40,d.data(),16);
    put(0x130,e.data(),16);const uint64_t zero{};put(0x210,&zero,8);put(0x120,e.data(),16);
    const uint32_t extent[2]{std::max(1u,engine.width),std::max(1u,engine.height)};put(0x110,&extent[0],4);put(0x140,&f,4);put(0x114,&extent[1],4);
    std::memset(reinterpret_cast<void*>(block+0x150),0,0x60);std::memset(reinterpret_cast<void*>(block+0x1B0),0,0x60);
    const uint8_t one=1;put(0x218,&one,1);
    return block;}
void __fastcall DispatchIndirect(uintptr_t o,uintptr_t args) noexcept {
    if(!OursOuter(o)){dispatch_indirect_original(o,args);return;}Call("dispatch_indirect");Bound("dispatch_args",args);Note(args);
    pending.args=args;Emit(Op::DispatchIndirect);}
void __fastcall DrawIndirect(uintptr_t o,uintptr_t args) noexcept {
    if(!OursOuter(o)){draw_indirect_original(o,args);return;}Call("draw_indirect");Bound("draw_args",args);Note(args);
    pending.args=args;Emit(Op::DrawIndirect);}
// The constant slots each statement reads (review: a slot it never binds takes no arena room and cannot drop it): a classic draw
// VS b0, DS b0, PS b0, b1, b6 (slot 27's); the particles' indirect draw GS b9, PS b0, b6, b9, b11 (the stage's particle root); a
// dispatch CS b0, b1, b5; a copy or clear none.
constexpr uint32_t Bit(ConstantSlot slot) {return 1u<<slot;}
constexpr uint32_t draw_slots=Bit(VsB0)|Bit(DsB0)|Bit(PsB0)|Bit(PsB1)|Bit(PsB6),particle_slots=Bit(GsB9)|Bit(PsB0)|Bit(PsB6)|Bit(PsB9)|Bit(PsB11),
    compute_slots=Bit(CsB0)|Bit(CsB1)|Bit(CsB5);
void Emit(Op op) {
    // Inside the engine's frames (the hooks are noexcept): a throw is kept and rethrown after Execute (review).
    try{
        pending.op=op;pending.pass=current_pass;pending.key=current_key;
        if(current_class==light_shaft_class)++(op==Op::Draw?shaft_command.draws:op==Op::ClearTarget?shaft_command.clears:op==Op::CopyTarget?shaft_command.copies:shaft_command.other);
        const std::array<uintptr_t,ConstantSlots> slots{pending.vs_constants[0],pending.ds_constants[0],pending.ps_constants[0],pending.ps_constants[1],
            pending.ps_constants[6],pending.gs_constants[9],pending.ps_constants[9],pending.ps_constants[11],pending.cs_constants[0],pending.cs_constants[1],
            pending.cs_constants[5]};
        const bool compute=op==Op::Dispatch || op==Op::DispatchIndirect;bool invalid=false;
        const uint32_t used=compute?compute_slots:op==Op::Draw?draw_slots:op==Op::DrawIndirect?particle_slots:0u;
        for(uint32_t i=0;i<ConstantSlots;++i)pending.constants[i]=used>>i&1?Place(slots[i],invalid):Range{};
        pending.constants_invalid=invalid;
        if(engine.witness && (op==Op::Draw || op==Op::DrawIndirect)){++draws[{current_pass,pending.topology}];
            char name[48]{};sprintf_s(name,"emit_%llX_%u",static_cast<unsigned long long>(current_class),pending.topology);++calls[name];
            // I-0105 (Lightning Spear's bolt, drawn by 606's class 3D7E380 as GX 11 patches, i105-spells-4): its first 8 draws whole:
            // the programmes and states bound, the counts, the first vertex's 32 bytes, the VS b0 head and the tracer programmes'
            // blocks whole (DS b0 DSCommonBuffer 448 bytes, PS b0 PSCommonBuffer 560, PS b1 PSLightBuffer's head, PS b6 cbFog 224),
            // as the Reference witness reference_ffx_tracer_draw prints them (04b_pass_witnesses.inc), to compare word by word; each
            // block with the bytes it was printed from (`<label>_bytes`, 0: not bound or not placed, its words printed as zeros) and the
            // draw's engine pass. Only its GX 11 draws, as the Reference row's.
            static int trail_rows{};
            if(current_class==0x3D7E380 && pending.topology==11 && trail_rows<8 && engine.log){++trail_rows;const auto& a=arena;std::string row;char cell[320]{};
                sprintf_s(cell,"{\"event\":\"native_ffx_trail_draw\",\"pass\":\"%X\",\"topology\":%u,\"count\":%u,\"start\":%u,\"indexed\":%d,\"stride\":%u,\"vertex_offset\":%u,\"vs\":%d,\"hs\":%d,\"ds\":%d,\"gs\":%d,\"ps\":%d,\"layout\":%d,\"invalid\":%d,\"v0\":[",
                    static_cast<unsigned>(current_pass),pending.topology,pending.count,pending.start,int(pending.indexed),pending.stride,pending.vertex_offset,pending.vs!=0,pending.hs!=0,pending.ds!=0,pending.gs!=0,pending.ps!=0,pending.layout!=0,int(invalid));row=cell;
                for(uint32_t i=0;i<8;++i){float f{};const size_t at=size_t(pending.vertex_offset)+size_t(pending.start)*pending.stride+4u*i;if(at+4<=a.size())std::memcpy(&f,a.data()+at,4);sprintf_s(cell,"%s%.4g",i?",":"",f);row+=cell;}
                struct Head {const char* label;ConstantSlot slot;uint32_t words;};
                for(const auto& [label,slot,words]:{Head{"vs_b0",VsB0,12},{"ds_b0",DsB0,112},{"ps_b0",PsB0,140},{"ps_b1",PsB1,4},{"ps_b6",PsB6,56}}){
                    const auto r=pending.constants[slot];sprintf_s(cell,"],\"%s_bytes\":%u,\"%s\":[",label,r.size,label);row+=cell;
                    for(uint32_t i=0;i<words;++i){float f{};const size_t at=size_t(r.offset)+4u*i;if(r.size>4u*i && at+4<=a.size())std::memcpy(&f,a.data()+at,4);sprintf_s(cell,"%s%.4g",i?",":"",f);row+=cell;}}
                row+="]}\n";engine.log(row.c_str());}
            // I-0140 (gap 16, the 608 radial blur's reach): the 608s' draws (class 3D7E8C0), the first 32 and every 500th after: the
            // counts, the first four vertices' first six floats and the VS b0 head (two matrices), to tell the quads' screen rectangles
            // (I-0113 tranche 31's witness read a zero matrix in another word).
            static uint64_t radial_draws{};
            if(current_class==0x3D7E8C0 && engine.log){const auto n=++radial_draws;if(n<=32 || n%500==0){const auto& a=arena;std::string row;char cell[320]{};
                sprintf_s(cell,"{\"event\":\"native_ffx_radial_draw\",\"n\":%llu,\"pass\":\"%X\",\"topology\":%u,\"count\":%u,\"start\":%u,\"base\":%d,\"indexed\":%d,\"stride\":%u,\"vertex_offset\":%u,\"invalid\":%d,\"v\":[",
                    static_cast<unsigned long long>(n),static_cast<unsigned>(current_pass),pending.topology,pending.count,pending.start,pending.base,int(pending.indexed),pending.stride,pending.vertex_offset,int(invalid));row=cell;
                for(uint32_t v=0;v<4;++v)for(uint32_t i=0;i<6;++i){float f{};const size_t at=size_t(pending.vertex_offset)+(size_t(pending.start)+v)*pending.stride+4u*i;
                    if(pending.stride>=24 && at+4<=a.size())std::memcpy(&f,a.data()+at,4);sprintf_s(cell,"%s%.5g",v||i?",":"",f);row+=cell;}
                const auto r=pending.constants[VsB0];sprintf_s(cell,"],\"vs_b0_bytes\":%u,\"vs_b0\":[",r.size);row+=cell;
                for(uint32_t i=0;i<32;++i){float f{};const size_t at=size_t(r.offset)+4u*i;if(r.size>4u*i && at+4<=a.size())std::memcpy(&f,a.data()+at,4);sprintf_s(cell,"%s%.5g",i?",":"",f);row+=cell;}
                row+="]}\n";engine.log(row.c_str());}}}
        if(op==Op::Draw && pending.topology==10 && engine.log && patch_rows_left.load()>0 && patch_rows_left.fetch_sub(1)>0){   // RequestPatchRows
            std::string row;char cell[96]{};sprintf_s(cell,"{\"event\":\"native_ffx_patch_draw\",\"count\":%u,\"indexed\":%d,\"ds\":[",pending.count,int(pending.indexed));row=cell;
            const auto words=[&](ConstantSlot slot,const char* bytes_key,uint32_t from,uint32_t to){const auto r=pending.constants[slot];for(uint32_t i=from;i<to;++i){float f{};const size_t at=size_t(r.offset)+4u*i;
                if(r.size>4u*i && at+4<=arena.size())std::memcpy(&f,arena.data()+at,4);sprintf_s(cell,"%s%.5g",i>from?",":"",f);row+=cell;}
                sprintf_s(cell,"],\"%s\":%u",bytes_key,r.size);row+=cell;};   // 0: not bound or not placed, its words printed as zeros
            words(DsB0,"ds_bytes",84,112);row+=",\"ps_scale\":[";words(PsB0,"ps_bytes",36,40);row+="}\n";engine.log(row.c_str());}
        if(emit_)(*emit_)(pending);   // the caller makes the native draw at once
    }catch(...){++emit_failures;if(!emit_failure)emit_failure=std::current_exception();}
    if(op==Op::Dispatch || op==Op::DispatchIndirect)pending.cs_uav_counts_set=0;}   // D3D11 applies the counts at the bind: a later dispatch keeps the counters
void __fastcall Draw(uintptr_t c,uintptr_t s,uint32_t count,uint32_t start) noexcept {                                                                          // vt+0x68
    if(!Ours(c)){draw_original(c,s,count,start);return;}Call("draw");pending.indexed=false;pending.count=count;pending.start=start;pending.base=0;Emit(Op::Draw);}
void __fastcall DrawIndexed(uintptr_t c,uintptr_t s,uint32_t count,uint32_t start,int32_t base) noexcept {                                                      // vt+0x60
    if(!Ours(c)){indexed_original(c,s,count,start,base);return;}Call("draw_indexed");pending.indexed=true;pending.count=count;pending.start=start;pending.base=base;Emit(Op::Draw);}
uint64_t __fastcall ContextType(uintptr_t object) noexcept {                                                                                                    // vt+0x380 GetType
    if(object==reinterpret_cast<uintptr_t>(&selector))return 1;   // deferred, as the original's executing contexts (Reference1953)
    return type_original(object);}
// E03 (I-0105, effect_draw.h ModelSubmission): on the native pass context, 29ADF0's two GX uploads have no device to reach (the scene
// constants 59470 through the model context +0x460, the arguments' commit 1A86C0) and the mesh loop 636F0 hands its inputs to the
// caller (Execute's `models`) instead of drawing; any other context runs the original. 29ADF0's arguments hold the context's address
// at +8 (1A8580 → 236290: args+8 = pass context + 0x460).
// The executing context is read by any thread that calls the three functions (only Execute's own thread matches it).
std::atomic<uintptr_t> executing_context{};const std::function<void(ModelSubmission&&)>* models_{};
bool OursModel(uintptr_t args) noexcept {const auto ours=executing_context.load(std::memory_order_acquire);uintptr_t holder{};
    return ours && args && engine.read(args+8,&holder,8) && holder==ours+0x460;}
using WScene=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);using WCommit=void(__fastcall*)(uintptr_t);using WModel=void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t);
WScene scene_constants_original{};WCommit commit_original{};WModel model_draw_original{};
void __fastcall SceneConstants(uintptr_t pass,uintptr_t args,uintptr_t a3,uintptr_t a4) noexcept {
    const auto ours=executing_context.load(std::memory_order_acquire);if(!ours || pass!=ours){scene_constants_original(pass,args,a3,a4);return;}Call("model_scene_constants");}
void __fastcall CommitArguments(uintptr_t args) noexcept {if(!OursModel(args)){commit_original(args);return;}Call("model_commit");}
void __fastcall ModelDraw(uintptr_t table,uintptr_t model,uintptr_t args,uintptr_t bones) noexcept {
    if(!OursModel(args)){model_draw_original(table,model,args,bones);return;}
    try{Call("model_draw");   // inside the engine's frames (the hook is noexcept): a throw is kept and rethrown after Execute, as Emit's
        uint8_t loaded{};uint32_t lods{};int32_t count{};
        if(!model || !engine.read(model+0x2B4,&loaded,1) || !loaded){Call("model_not_loaded");return;}   // 636F0 returns
        ModelSubmission m;m.model=model;m.bones=bones;m.pass=current_pass;m.key=current_key;m.renderer=current_class;m.skipped_origin=skipped_origin;
        for(const auto& [token,offset]:{std::pair{pending.blend,&m.blend},{pending.depth,&m.depth}})   // the context's bound states
            if(const auto* t=AsToken(token);t && t->magic==state_magic)*offset=t->value;
        if(m.blend)m.blend_origin=blend_origin;if(m.depth)m.depth_origin=depth_origin;   // I-0113 gap 5: native_effect_model_first prints them
        if(engine.witness){const auto shown=static_cast<size_t>(std::min<uint64_t>(command_count,command_ring.size()));
            for(size_t i=0;i<shown;++i)m.sequence.push_back(command_ring[(command_count-shown+i)%command_ring.size()]);}
        if(!engine.read(args+0x18,&m.lod,4) || !engine.read(args+0x1A8,&m.materials,8) || !engine.read(args+0x200,&m.arg_flags,4) ||
           !engine.read(args+0x188,&m.arg_depth,4) || !engine.read(args+0x180,&m.arg_cull,4) || !engine.read(args+0x184,&m.arg_cull_mode,4) ||
           !engine.read(args+0x240,m.state.data(),m.state.size()) ||
           !engine.read(args+0x3E0,m.light.data(),m.light.size())){Call("model_args_unreadable");return;}
        // I-0113 gap 17 (final review H2): 5A120, which the native 59470 detour skips, sets bit 14 when the pass's +0x714 is 1 and
        // bit 15 when it is 2 (5A120:370-381); the native pass context has no +0x714, and the original's effect pass showed neither
        // (no alpha-to-coverage in its 1,355 crystal and flask draws, i113-t9-*-oracle*): both clear.
        m.arg_flags&=~0xC000u;
        if(engine.read(model+0x1E4,&lods,4) && lods && m.lod>=lods)m.lod=lods-1;   // 636F0's clamp
        if(!engine.read(model+0x1C,&count,4) || count<0 || count>4096){Call("model_bones_unreadable");return;}
        if(count){m.rows.resize(static_cast<size_t>(count));
            if(!bones || !engine.read(bones,m.rows.data(),m.rows.size()*sizeof(m.rows[0]))){Call("model_bones_unreadable");return;}}
        if(models_)(*models_)(std::move(m));
    }catch(...){++emit_failures;if(!emit_failure)emit_failure=std::current_exception();}}
}

uintptr_t TextureToken(uintptr_t receiver) {
    Statics();const std::lock_guard hold(token_lock);auto& slot=texture_tokens[receiver];
    if(!slot){slot=std::make_unique<Token>();slot->vtable=Traps();slot->magic=texture_magic;slot->payload=receiver;Remember(slot.get());}
    return reinterpret_cast<uintptr_t>(slot.get());}
namespace {std::unordered_map<uintptr_t,std::unique_ptr<Token>> program_tokens;thread_local int program_scope{};}
uintptr_t ProgramToken(uintptr_t shader_object) {
    Statics();const std::lock_guard hold(token_lock);auto& slot=program_tokens[shader_object];
    if(!slot){slot=std::make_unique<Token>();slot->vtable=Traps();slot->magic=program_magic;slot->payload=shader_object;Remember(slot.get());}
    return reinterpret_cast<uintptr_t>(slot.get());}
ProgramScope::ProgramScope() noexcept {++program_scope;}
ProgramScope::~ProgramScope() {--program_scope;}
bool ProgramScopeOpen() noexcept {return program_scope>0;}
const Token* AsToken(uintptr_t pointer) noexcept {
    const std::lock_guard hold(token_lock);
    return token_addresses.count(pointer)?reinterpret_cast<const Token*>(pointer):nullptr;}
uintptr_t StateBank() noexcept {Statics();return reinterpret_cast<uintptr_t>(bank.data());}
uintptr_t DefaultTextureProvider() noexcept {Statics();return reinterpret_cast<uintptr_t>(&provider);}
uintptr_t FrameCopyTexture() noexcept {Statics();return reinterpret_cast<uintptr_t>(copy_texture.data());}
uintptr_t DepthTexture() noexcept {Statics();return reinterpret_cast<uintptr_t>(depth_texture.data());}
uintptr_t DeviceShadow() noexcept {return reinterpret_cast<uintptr_t>(device_shadow.data());}
// D3: sequentially consistent on both sides (the users count before the flag here, the flag before the count in Execute).
// A thread already inside a use (a nested call) is let in at once (the step review's L1: Execute waits for its count).
std::atomic<int> engine_sh_users{};std::atomic<bool> shadow_named{};thread_local bool executing_record{};thread_local int engine_sh_depth{};
bool EnterEngineShUse() noexcept {engine_sh_users.fetch_add(1);
    if(engine_sh_depth || !shadow_named.load()){++engine_sh_depth;return true;}
    engine_sh_users.fetch_sub(1);return false;}
void LeaveEngineShUse() noexcept {--engine_sh_depth;engine_sh_users.fetch_sub(1);}
// Bounded (the final review's L1: an engine lock held by the waiter and needed by the record would hang both sides): past 2 s the
// caller goes on as before D3 and is told (true), counted in, so Execute's naming waits for it to leave. Execute's own wait for the
// users stays unbounded: naming nothing would let the GPU particles read the null member (Native1975; that answer's step review H1).
bool WaitEngineShUse() noexcept {
    const auto until=GetTickCount64()+2000;
    while(!EnterEngineShUse()){
        if(executing_record || GetTickCount64()>=until){engine_sh_users.fetch_add(1);++engine_sh_depth;return !executing_record;}   // as before D3
        SwitchToThread();}
    return false;
}
const std::vector<uint8_t>& Arena() noexcept {return arena;}
std::array<uint32_t,2> TargetSize() noexcept {return {std::max(1u,engine.width/2),std::max(1u,engine.height/2)};}
void RequestPatchRows(int n) noexcept {patch_rows_left.store(n);}
void RequestVsRows(int n) noexcept {vs_rows_left.store(n);}
size_t ArenaUsed() noexcept {return arena_used;}
uint32_t Append(const void* bytes,size_t size,size_t alignment) {
    const auto at=Reserve(size,alignment);if(at==UINT32_MAX){++arena_refusals;return at;}
    if(size)std::memcpy(arena.data()+at,bytes,size);return at;}
namespace {std::mutex forget_lock;std::vector<uintptr_t> forgotten;std::vector<uint64_t> released_serials;}
void Forget(uintptr_t handle,uint64_t serial) {try{const std::lock_guard hold(forget_lock);forgotten.push_back(handle);if(serial)released_serials.push_back(serial);}catch(...){}}
std::vector<uint64_t> TakeReleased() {const std::lock_guard hold(forget_lock);std::vector<uint64_t> out;out.swap(released_serials);return out;}
void GiveBack(const std::vector<uint64_t>& serials) {try{const std::lock_guard hold(forget_lock);released_serials.insert(released_serials.begin(),serials.begin(),serials.end());}catch(...){}}

std::vector<Hook> Hooks() {
    return {{0x1920300,reinterpret_cast<void*>(&Update),reinterpret_cast<void**>(&update_original)},
        {0x19203F0,reinterpret_cast<void*>(&Layout),reinterpret_cast<void**>(&layout_original)},
        {0x1920420,reinterpret_cast<void*>(&Topology),reinterpret_cast<void**>(&topology_original)},
        {0x1920460,reinterpret_cast<void*>(&IndexBuffer),reinterpret_cast<void**>(&index_original)},
        {0x19204C0,reinterpret_cast<void*>(&VertexBuffers),reinterpret_cast<void**>(&vertex_original)},
        {0x1920660,reinterpret_cast<void*>(&VsConstants),reinterpret_cast<void**>(&vs_cb_original)},
        {0x19206E0,reinterpret_cast<void*>(&Vs),reinterpret_cast<void**>(&vs_original)},
        {0x19208A0,reinterpret_cast<void*>(&Hs),reinterpret_cast<void**>(&hs_original)},
        {0x19209D0,reinterpret_cast<void*>(&DsConstants),reinterpret_cast<void**>(&ds_cb_original)},
        {0x1920A50,reinterpret_cast<void*>(&Ds),reinterpret_cast<void**>(&ds_original)},
        {0x1920C00,reinterpret_cast<void*>(&Gs),reinterpret_cast<void**>(&gs_original)},
        {0x1920CA0,reinterpret_cast<void*>(&Raster),reinterpret_cast<void**>(&raster_original)},
        {0x1920D20,reinterpret_cast<void*>(&PsSamplers),reinterpret_cast<void**>(&ps_sampler_original)},
        {0x1920DA0,reinterpret_cast<void*>(&PsResources),reinterpret_cast<void**>(&ps_srv_original)},
        {0x1920E20,reinterpret_cast<void*>(&PsConstants),reinterpret_cast<void**>(&ps_cb_original)},
        {0x1920EA0,reinterpret_cast<void*>(&Ps),reinterpret_cast<void**>(&ps_original)},
        {0x1920ED0,reinterpret_cast<void*>(&Blend),reinterpret_cast<void**>(&blend_original)},
        {0x1920F00,reinterpret_cast<void*>(&Depth),reinterpret_cast<void**>(&depth_original)},
        {0x1921370,reinterpret_cast<void*>(&Draw),reinterpret_cast<void**>(&draw_original)},
        {0x1921390,reinterpret_cast<void*>(&DrawIndexed),reinterpret_cast<void**>(&indexed_original)},
        {0x1921AC0,reinterpret_cast<void*>(&ContextType),reinterpret_cast<void**>(&type_original)},
        {0x1920200,reinterpret_cast<void*>(&ClearRtv),reinterpret_cast<void**>(&clear_rtv_original)},
        {0x1920F40,reinterpret_cast<void*>(&RenderTargets),reinterpret_cast<void**>(&targets_original)},
        {0x0C4F10,reinterpret_cast<void*>(&StateBlock),reinterpret_cast<void**>(&state_block_original)},
        {0x1920CD0,reinterpret_cast<void*>(&Viewports),reinterpret_cast<void**>(&viewports_original)},
        {0x1920CF0,reinterpret_cast<void*>(&Scissors),reinterpret_cast<void**>(&scissors_original)},
        {0x1920220,reinterpret_cast<void*>(&ClearUav),reinterpret_cast<void**>(&clear_uav_original)},
        {0x1920260,reinterpret_cast<void*>(&Copy),reinterpret_cast<void**>(&copy_original)},
        {0x1920B80,reinterpret_cast<void*>(&GsConstants),reinterpret_cast<void**>(&gs_cb_original)},
        {0x19205E0,reinterpret_cast<void*>(&VsResources),reinterpret_cast<void**>(&vs_srv_original)},
        {0x1921160,reinterpret_cast<void*>(&CsResources),reinterpret_cast<void**>(&cs_srv_original)},
        {0x1921260,reinterpret_cast<void*>(&CsUavs),reinterpret_cast<void**>(&cs_uav_original)},
        {0x19212F0,reinterpret_cast<void*>(&Cs),reinterpret_cast<void**>(&cs_original)},
        {0x19211E0,reinterpret_cast<void*>(&CsConstants),reinterpret_cast<void**>(&cs_cb_original)},
        {0x1921320,reinterpret_cast<void*>(&Dispatch),reinterpret_cast<void**>(&dispatch_original)},
        {0x0C5F40,reinterpret_cast<void*>(&DispatchIndirect),reinterpret_cast<void**>(&dispatch_indirect_original)},
        {0x0C61B0,reinterpret_cast<void*>(&DrawIndirect),reinterpret_cast<void**>(&draw_indirect_original)},
        {0x059470,reinterpret_cast<void*>(&SceneConstants),reinterpret_cast<void**>(&scene_constants_original)},
        {0x1A86C0,reinterpret_cast<void*>(&CommitArguments),reinterpret_cast<void**>(&commit_original)},
        {0x0636F0,reinterpret_cast<void*>(&ModelDraw),reinterpret_cast<void**>(&model_draw_original)},
        {0x260440,reinterpret_cast<void*>(&CopyRect),reinterpret_cast<void**>(&copy_rect_original)},
        {0x262A70,reinterpret_cast<void*>(&TrailAppearance),reinterpret_cast<void**>(&appearance_original)}};
}

uint64_t Execute(const Engine& e,uintptr_t pass_context,uintptr_t sc,std::vector<Command> commands,const std::function<void(const EffectDrawRecord&)>& emit,
    const std::function<void(ModelSubmission&&)>& models) {
    engine=e;Statics();
    {static bool listed{};if(e.gpu_particles && !listed && e.log){listed=true;char row[700]{};   // E07 diagnosis: where the native objects live
        sprintf_s(row,"{\"event\":\"native_ffx_objects\",\"bank\":%llu,\"states\":%llu,\"default\":%llu,\"ring_token\":%llu,\"copy_token\":%llu,\"depth_token\":%llu,\"provider\":%llu,\"provider_object\":%llu,\"copy_texture\":%llu,\"depth_texture\":%llu,\"trap_object\":%llu,\"selector\":%llu,\"ring_holder\":%llu,\"outer\":%llu,\"inner\":%llu,\"traps\":%llu}\n",
            (unsigned long long)bank.data(),(unsigned long long)state_tokens.data(),(unsigned long long)&default_token,(unsigned long long)&ring_token,(unsigned long long)&copy_token,(unsigned long long)&depth_token,
            (unsigned long long)&provider,(unsigned long long)&provider_object,(unsigned long long)copy_texture.data(),(unsigned long long)depth_texture.data(),(unsigned long long)&trap_object,
            (unsigned long long)&selector,(unsigned long long)&ring_holder,(unsigned long long)outer.data(),(unsigned long long)inner.data(),(unsigned long long)Traps());e.log(row);}}
    if(!trap_object.vtable){trap_object.vtable=Traps();selector.object=reinterpret_cast<uintptr_t>(&trap_object);
        std::copy_n(trap_vtable.begin(),ring_vtable.size(),ring_vtable.begin());ring_vtable[1]=reinterpret_cast<uintptr_t>(&RingMap);ring_vtable[2]=reinterpret_cast<uintptr_t>(&RingUnmap);
        ring_object.vtable=reinterpret_cast<uintptr_t>(ring_vtable.data());ring_holder.object=reinterpret_cast<uintptr_t>(&ring_object);}
    // The contexts: zeroed each frame (INFERRED: the original's deferred contexts start each recording with fresh caches), then linked.
    std::memset(outer.data(),0,outer.size());std::memset(inner.data(),0,inner.size());
    *reinterpret_cast<uintptr_t*>(inner.data())=reinterpret_cast<uintptr_t>(&trap_object);
    *reinterpret_cast<uintptr_t*>(outer.data()+8)=Inner();
    // The outer context's viewport cache (+0xC8 the count, +0xCC D3D11_VIEWPORTs): the draw-context setups 263C70 and 264230 divide
    // a PSCommonBuffer row by its width and height (a zero cache gave NaN at PS b0 +0x1E0, Native1996). One viewport, the scene's.
    if(e.width && e.height){const uint32_t count=1;const float viewport[6]{0.f,0.f,float(e.width),float(e.height),0.f,1.f};
        std::memcpy(outer.data()+0xC8,&count,4);std::memcpy(outer.data()+0xCC,viewport,sizeof(viewport));
        // The inner context's viewport cache (+0x4D8 the count, +0x258 the viewports): 375DF0 passes the first one's width and height
        // to the draw context's 263330 (its +0x270..+0x27C: g_screenSize and g_recScreenSize, VSCommonBuffer +0x130..+0x13F). Empty natively, the
        // point sprites (the m37 snow) had a zero size (Native2016; the original: 1280, 720 and their reciprocals, Reference2018/2020).
        std::memcpy(inner.data()+0x4D8,&count,4);std::memcpy(inner.data()+0x258,viewport,sizeof(viewport));}
    // I-0140 tranche 10: the targets' descriptions (D3D11_TEXTURE2D_DESC words: width, height, one mip, one slice, the format, one
    // sample, default usage, bound as shader resource and render target), the scene's at the scene's size (R16G16B16A16_FLOAT,
    // INFERRED: only its size is read, by 271B40's scissor), the half-size ones as 24F2C0 makes them (R8G8B8A8_UNORM, half the view's
    // size). The outer context's render-target cache names the scene's view during each light-shaft command only (the command loop).
    {const auto half=TargetSize();
        for(uint32_t i=0;i<effect_targets;++i)target_words[i]={i?half[0]:std::max(1u,e.width),i?half[1]:std::max(1u,e.height),1,1,i?0x1Cu:0x0Au,1,0,0,0x28,0,0,0};
    }
    // The scene control's +0x238 and +0x240 (null natively: 24F2C0 does not run) name the half-size targets for the execution only,
    // null again before Execute returns (only 271B40 reads them in the executable besides the lifecycle code that does not run natively).
    struct TargetSlots {uintptr_t at{};~TargetSlots(){if(at){*reinterpret_cast<uintptr_t*>(at)=0;*reinterpret_cast<uintptr_t*>(at+8)=0;}}} target_slots;
    if(e.light_shafts && !*reinterpret_cast<const uintptr_t*>(sc+0x238) && !*reinterpret_cast<const uintptr_t*>(sc+0x240)){target_slots.at=sc+0x238;
        *reinterpret_cast<uintptr_t*>(sc+0x238)=reinterpret_cast<uintptr_t>(&target_objects[1]);*reinterpret_cast<uintptr_t*>(sc+0x240)=reinterpret_cast<uintptr_t>(&target_objects[2]);}
    else if(e.light_shafts && e.log){static bool told{};if(!told){told=true;e.log("{\"event\":\"native_ffx_light_shaft_slots_taken\"}\n");}}
    *reinterpret_cast<uintptr_t*>(pass_context+0x450)=reinterpret_cast<uintptr_t>(outer.data());
    *reinterpret_cast<uintptr_t*>(pass_context+0x458)=reinterpret_cast<uintptr_t>(&selector);
    // The ring of context 0: sc+0xD0 is the native one-slot table (DeviceInitialisation); only the multi-threaded paths, which do
    // not run natively, read it otherwise.
    if(const auto slots=*reinterpret_cast<const uintptr_t*>(sc+0xD0))*reinterpret_cast<uintptr_t*>(slots)=reinterpret_cast<uintptr_t>(&ring_holder);
    // E07 (gpu_particles): the GPU-particle draw 2A7E70 reads cbCubeEnvSH1 at [[GXRS]+0x4DD0]+0x2C0, the scene root's device member
    // that stays null natively (other native readers rely on that). For the execution only, the member names a native settings
    // shadow (zero but for +0x2C0, a 48-byte native effect buffer); it is null again before Execute returns. The write is process-
    // wide and, since I-0088, made from the collect's side worker while the walk runs: the module's own readers take the shadow as
    // absent (DeviceShadow, NativeDeviceAbsent).
    auto& settings_shadow=device_shadow;static uintptr_t root_member{};
    if(e.gpu_particles && !*reinterpret_cast<const uintptr_t*>(settings_shadow.data()+0x2C0)){
        const std::array<uint32_t,6> description{48,0,4,0,0,0};uintptr_t buffer{};
        if(!effect_system::CreateBuffer(reinterpret_cast<uintptr_t>(&buffer),description.data(),0))*reinterpret_cast<uintptr_t*>(settings_shadow.data()+0x2C0)=buffer;}
    root_member=0;
    // D3: named once no engine body that tests the member runs elsewhere (EnterEngineShUse), and those wait or take their
    // device-absent path until it is null again.
    struct Named {bool on{};~Named(){if(on){shadow_named.store(false);executing_record=false;}}} named;
    if(e.gpu_particles){uintptr_t root{},prior{};named.on=true;executing_record=true;shadow_named.store(true);
        while(engine_sh_users.load())SwitchToThread();
        if(e.read(e.image+0x4961B50,&root,8) && root && e.read(root+0x4DD0,&prior,8) && !prior){root_member=root+0x4DD0;
            *reinterpret_cast<uintptr_t*>(root_member)=reinterpret_cast<uintptr_t>(settings_shadow.data());}
        if(!root_member){shadow_named.store(false);executing_record=false;named.on=false;}}   // the step review's L2: nothing named, nobody waits
    struct Restore {uintptr_t& at;~Restore(){if(at){*reinterpret_cast<uintptr_t*>(at)=0;at=0;}}} restore{root_member};   // before `named` ends
    if(arena_full){arena_full=false;   // review: the last frame ran out of the arena: twice the room, said once a size (and once at the limit)
        static bool at_limit_told{};const bool grow=arena.size()<arena_limit;
        if(grow)arena.resize(std::min(arena.size()*2,arena_limit));
        if(e.log && (grow || !at_limit_told)){at_limit_told|=!grow;char row[200]{};
            sprintf_s(row,"{\"event\":\"native_ffx_arena_grown\",\"bytes\":%zu,\"at_limit\":%s,\"ring_refusals\":%llu,\"arena_refusals\":%llu}\n",arena.size(),grow?"false":"true",
                static_cast<unsigned long long>(ring_refusals),static_cast<unsigned long long>(arena_refusals));e.log(row);}}
    pending=EffectDrawRecord{};blend_origin=depth_origin=skipped_origin={};command_count=0;arena_used=zero_bytes;block_start=0;placed.clear();emit_failure=nullptr;
    emit_=&emit;models_=&models;executing_context=pass_context;struct Unset {~Unset(){emit_=nullptr;models_=nullptr;executing_context=0;}} unset;
    {const std::lock_guard hold(forget_lock);for(const auto h:forgotten){constant_bytes.erase(h);invalid_constants.erase(h);}forgotten.clear();}
    constexpr int32_t order[]{0x21,0x29,0x2D,0x20,0x31};
    const auto rank=[&](int32_t pass){for(int32_t i=0;i<5;++i)if(order[i]==pass)return i;return 5;};
    std::stable_sort(commands.begin(),commands.end(),[&](const Command& a,const Command& b){
        const auto ra=rank(a.pass),rb=rank(b.pass);return ra!=rb?ra<rb:a.key<b.key;});
    // The families executed: a command's entity is the node's draw item (node+0x78, its vt+8 141ADC070 calls the node's renderer
    // [node+0x58] slot 8); only the allowed renderer classes run: the E01/E02 builders (37D260's 3D7E5C0, 375C70's 3D7E440, 385DF0's
    // 3D7E680, 396DA0's 3D7E800), the GXVfx appearances 3D65878 and 3D65CC8 (E07, on by default since D-0186), and those below; any
    // other class or entity kind is counted and skipped.
    const uintptr_t node_item=e.image+0x1ADC070;uint64_t run{};
    // Spells (Native2034 against Reference2032, Fire Orb): 39E900's class 3D7E8C0 (the Draw(4) strips of pass 0x2D) and 392560's class
    // 3D7E740 run as the other builders (ring, slot 27); 27EA70's class 3D65F88 (E03, the model particles: one a frame at Firelink) by
    // default since I-0105, its FLVER draws handed to `models` (ModelDraw).
    // 378F50's class 3D7E4A0 (the same ring and slot 27, a census of the renderer vtables sharing slot 11 = 267B20) too, not yet witnessed (Native2046-2049 carried it, no fault).
    // The trails (I-0105 tranche 2): 3709B0's class 3D7E380 (606) and 2891A0's class 3D660B8 (10012, skipped at m30 in Native1968) set
    // up the draw context as the builders (its slots 0x58, 0xA0, 8, 0xB0, 0x20, 0x28, 0x60 and 27) and write their strips through
    // 36F280 / 36F350 on the context's inner GX context [drawCtx+0x20] and ring holder [drawCtx+0x28] (the map through 213B690, then the
    // topology, vertex buffer and draw wrappers): the native context's own statements.
    const uintptr_t allowed[]{e.image+0x3D7E5C0,e.image+0x3D7E440,e.image+0x3D7E680,e.image+0x3D7E800,e.image+0x3D7E8C0,e.image+0x3D7E740,e.image+0x3D7E4A0,
        e.gpu_particles?e.image+0x3D65878:0,e.gpu_particles?e.image+0x3D65CC8:0,e.model_particles?e.image+0x3D65F88:0,
        e.trails?e.image+0x3D7E380:0,e.trails?e.image+0x3D660B8:0,
        // I-0140 tranche 10: 10003's light shafts (271B40), with their own targets (the slots above; without them it is skipped).
        e.light_shafts && target_slots.at?e.image+light_shaft_class:0};
    // I-0113 gap 5 (PASS-CHUNKS-STUDY, RESULT): the original's sub 0x29 interleaves type-3 model rows with these commands, so a model
    // particle may follow a model row there: each submission carries the commands that bound its blend and depth (ResolveNative-
    // EffectRowStates in the bootstrap takes a row's state keyed after them); the chunks are not reproduced. Witness (ffx-record.flag):
    // the number of 0x29 commands every 300th execution.
    if(e.witness && e.log && frames%300==0){const auto n=std::count_if(commands.begin(),commands.end(),[](const Command& c){return c.pass==0x29;});
        char row[96]{};sprintf_s(row,"{\"event\":\"native_effect_chunks\",\"n\":%lld}\n",static_cast<long long>(n));e.log(row);}
    for(const auto& c:commands){
        if(!c.callback || !c.data){skipped_origin={c.pass,c.key,0};continue;}
        uintptr_t table{},slot{},renderer{},renderer_table{};
        // I-0113 tranche 23 (STUDY-SILENT-CALLS row 3): each new kind of skipped command logged once by default (at most 64 a session),
        // so a class the witness zones never showed is loud in an ordinary run; the counts stay the witness's.
        const auto tell=[&](const char* what,uint64_t value){if(!e.log || skipped_told.size()>=64 || !skipped_told.insert(uint64_t(what[0])<<56^value).second)return;   // a key space per kind
            char row[160]{};sprintf_s(row,"{\"event\":\"native_ffx_skipped\",\"what\":\"%s\",\"value\":\"%llX\",\"pass\":\"%X\"}\n",what,static_cast<unsigned long long>(value),unsigned(c.pass));e.log(row);};
        if(!e.read(c.entity,&table,8) || !e.read(table+8,&slot,8) || slot!=node_item){Call("skipped_entity_kind");tell("entity_kind",slot>e.image?slot-e.image:slot);skipped_origin={c.pass,c.key,0};continue;}
        if(!e.read(c.entity-0x78+0x58,&renderer,8) || !renderer || !e.read(renderer,&renderer_table,8) || !renderer_table ||   // review: the list holds 0 for a flag off
           std::find(std::begin(allowed),std::end(allowed),renderer_table)==std::end(allowed)){
            tell("renderer_class",renderer_table?renderer_table-e.image:0);
            if(e.witness){char name[48]{};sprintf_s(name,"skipped_renderer_%llX",static_cast<unsigned long long>(renderer_table?renderer_table-e.image:0));++calls[name];}
            skipped_origin={c.pass,c.key,renderer_table?renderer_table-e.image:0};continue;}
        // I-0140 tranche 10: 271B40 takes its targets from the command's scene control (data+0x10, which 299E40 hands it through the
        // draw context's +0x10) and dereferences them without a test in its blur: run only where that is the scene control whose slots
        // name ours.
        if(renderer_table==e.image+light_shaft_class){uintptr_t owner{};
            if(!e.read(c.data+0x10,&owner,8) || owner!=sc){Call("skipped_light_shaft_owner");tell("light_shaft_owner",owner);
                if(const auto n=++shaft_owner_skips;e.log && (n<=4 || n%600==0)){char row[120]{};   // the second step review's M2: in every run
                    sprintf_s(row,"{\"event\":\"native_ffx_light_shaft_owner_skipped\",\"n\":%llu,\"pass\":\"%X\"}\n",static_cast<unsigned long long>(n),unsigned(c.pass));e.log(row);}
                skipped_origin={c.pass,c.key,light_shaft_class};continue;}}
        *reinterpret_cast<int32_t*>(pass_context+0x128)=c.pass;*reinterpret_cast<uint32_t*>(pass_context+0x12C)=c.key;
        current_pass=c.pass;current_key=c.key;current_class=renderer_table-e.image;
        if(e.witness){char name[40]{};sprintf_s(name,"run_%llX",static_cast<unsigned long long>(current_class));++calls[name];   // I-0105: the commands run by class
            command_ring[command_count++%command_ring.size()]={{c.pass,c.key,current_class},0};}
        // The step reviews' M2 and M1: what a light-shaft command leaves bound for the next commands is restored after it to what it
        // was before it: the record's target, viewport and scissor (271B40 never resets its scissor rectangle; an early return after
        // its first bind, a ring map refused, leaves target 1 bound and the half-size viewport set) and the GX contexts' caches the
        // other families' setups read (the outer's render targets and their depth view +0x10..+0x5F, its 8 viewports and 8 scissors +0xC8..+0x20F, which
        // the depth-stencil state follows at +0x210; the inner's viewports and scissors +0x258..+0x4DF, render targets and depth view +0x4F8..+0x53F and their count +0x59D: 263C70, 264230 and
        // 375DF0 divide by the viewports). Its other state (programmes, blend, the depth-stencil state outer +0x210/+0x218 and inner
        // +0x4F0/+0x59C, raster, layout, topology, vertex buffers) stays as it set it in both contexts, as the original leaves it; the
        // E03 model submissions after it inherit its blend and depth (ModelDraw), as there.
        struct Kept {uint8_t target;bool viewport_set,scissor_set;std::array<float,6> viewport;std::array<int32_t,4> scissor;
            std::array<uint8_t,0x50> outer_targets;std::array<uint8_t,0x148> outer_rects;std::array<uint8_t,0x288> inner_rects;std::array<uint8_t,0x48> inner_targets;uint8_t inner_count;};
        const bool shaft=current_class==light_shaft_class;
        std::optional<Kept> kept;   // the third step review's L1: no allocation
        if(shaft){kept.emplace();kept->target=pending.target;kept->viewport_set=pending.viewport_set;kept->scissor_set=pending.scissor_set;
            kept->viewport=pending.viewport;kept->scissor=pending.scissor;
            std::memcpy(kept->outer_targets.data(),outer.data()+0x10,0x50);std::memcpy(kept->outer_rects.data(),outer.data()+0xC8,0x148);
            std::memcpy(kept->inner_rects.data(),inner.data()+0x258,0x288);std::memcpy(kept->inner_targets.data(),inner.data()+0x4F8,0x48);kept->inner_count=inner[0x59D];
            // The outer render-target cache (+0x10 the count, +0x18 the views, as SetRenderTargetsWrapper 5F00 writes them) names the
            // scene's view, the effect pass's target, which 271B40 saves and binds again after its own; the restore below removes it
            // (the final review's L1: other families see the cache as before the tranche).
            const uint32_t one=1;const auto scene_view=reinterpret_cast<uintptr_t>(&target_tokens[0][size_t(TargetRole::Rtv)]);
            std::memcpy(outer.data()+0x10,&one,4);std::memcpy(outer.data()+0x18,&scene_view,8);
            shaft_command={};}
        reinterpret_cast<void(__fastcall*)(uintptr_t,uintptr_t,uintptr_t)>(c.callback)(pass_context,c.entity,c.data);
        if(kept){pending.target=kept->target;pending.viewport_set=kept->viewport_set;pending.scissor_set=kept->scissor_set;pending.viewport=kept->viewport;pending.scissor=kept->scissor;
            std::memcpy(outer.data()+0x10,kept->outer_targets.data(),0x50);std::memcpy(outer.data()+0xC8,kept->outer_rects.data(),0x148);
            std::memcpy(inner.data()+0x258,kept->inner_rects.data(),0x288);std::memcpy(inner.data()+0x4F8,kept->inner_targets.data(),0x48);inner[0x59D]=kept->inner_count;}
        // I-0140 tranche 10: each light-shaft command's statements (the first 8 and every 600th, rate-limited in every run: the tour's
        // journals carry no ffx-record), its pass and the half-size targets' size.
        if(current_class==light_shaft_class && e.log){const auto n=++shaft_commands;if(n<=8 || n%600==0){const auto half=TargetSize();char row[220]{};
            sprintf_s(row,"{\"event\":\"native_ffx_light_shaft\",\"n\":%llu,\"pass\":\"%X\",\"draws\":%u,\"clears\":%u,\"copies\":%u,\"other\":%u,\"targets\":[%u,%u],\"owner_skips\":%llu}\n",
                static_cast<unsigned long long>(n),unsigned(c.pass),shaft_command.draws,shaft_command.clears,shaft_command.copies,shaft_command.other,half[0],half[1],
                static_cast<unsigned long long>(shaft_owner_skips));e.log(row);}}
        ++run;}
    ++frames;commands_run+=run;
    if(emit_failure){const auto failure=emit_failure;emit_failure=nullptr;std::rethrow_exception(failure);}   // outside the engine's frames
    return run;
}
std::string Summary() {
    std::string out="{\"frames\":"+std::to_string(frames)+",\"commands\":"+std::to_string(commands_run)+",\"draws\":{";
    bool first=true;
    for(const auto& [k,v]:draws){char cell[64]{};sprintf_s(cell,"%s\"%X/%u\":%llu",first?"":",",k.first,k.second,static_cast<unsigned long long>(v));out+=cell;first=false;}
    out+="},\"calls\":{";first=true;
    for(const auto& [k,v]:calls){out+=(first?"\"":",\"")+k+"\":"+std::to_string(v);first=false;}
    out+="},\"bound\":{";first=true;
    for(const auto& [k,v]:bound){out+=(first?"\"":",\"")+k+"\":"+std::to_string(v);first=false;}
    out+="},\"traps\":{";first=true;
    for(size_t i=0;i<trap_calls.size();++i)if(const auto n=trap_calls[i].load()){out+=(first?"\"":",\"")+std::to_string(i)+"\":"+std::to_string(n);first=false;}
    out+="},\"arena_peak\":"+std::to_string(arena_peak)+",\"ring_refusals\":"+std::to_string(ring_refusals)+",\"arena_refusals\":"+std::to_string(arena_refusals)+",\"emit_failures\":"+std::to_string(emit_failures)+"}";
    draws.clear();calls.clear();bound.clear();frames=0;commands_run=0;arena_peak=0;
    return out;
}
}
