// W4 FW-1: see ffx_observe.h. Rows `native_ffx_door`: the first three graphics steps, then every 600th, each with the
// counts of the window since the previous row (calls, callers by return address, effect identifiers and flags of the
// creators, the creators' `+0x58` branch, the live node count, the order of the effect update and the light collect).
#include "ffx_observe.h"
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

namespace ds3native::ffx_observe {
namespace {
std::atomic<bool> enabled{false};
uintptr_t image{};
ReadFn read{};
LogFn log{};
const char* mode="native";
SRWLOCK lock=SRWLOCK_INIT;
std::array<Window,EntryCount> window{};
uint64_t frames{},row_frame{};
uint32_t nodes_last{},nodes_max{};
uint64_t sequence{},update_sequence{},collect_sequence{},update_first{},collect_alone{};
std::array<void*,EntryCount> originals{};
BYTE* page{};
std::once_flag page_once;

uint32_t Dword(uintptr_t address,bool& ok) noexcept {
    uint32_t value{};ok=address && read(address,&value,4);return ok?value:0;
}
template<size_t N> void Append(std::string& out,const char* name,const Histogram<N>& h,bool hex) {
    out+=",\"";out+=name;out+="\":[";
    for(size_t i=0;i<h.used;++i){char cell[64]{};
        sprintf_s(cell,hex?"%s[\"%llX\",%llu]":"%s[%llu,%llu]",i?",":"",static_cast<unsigned long long>(h.keys[i]),
            static_cast<unsigned long long>(h.counts[i]));out+=cell;}
    out+="]";
    if(h.other){char cell[48]{};sprintf_s(cell,",\"%s_other\":%llu",name,static_cast<unsigned long long>(h.other));out+=cell;}
}
// Under the lock: one row for the window that ends here, then a fresh window.
void Row() {
    std::string out;out.reserve(4096);
    char head[256]{};sprintf_s(head,"{\"event\":\"native_ffx_door\",\"mode\":\"%s\",\"frame\":%llu,\"window_frames\":%llu",
        mode,static_cast<unsigned long long>(frames),static_cast<unsigned long long>(frames-row_frame));out+=head;
    for(const auto& site:sites){
        const auto& w=window[site.entry];
        char cell[160]{};sprintf_s(cell,",\"%s\":{\"calls\":%llu",site.name,static_cast<unsigned long long>(w.calls));out+=cell;
        if(site.entry<=CutsceneCreate || site.entry==SfxUpdate){sprintf_s(cell,",\"system\":%llu",static_cast<unsigned long long>(w.system));out+=cell;}
        Append(out,"callers",w.callers,true);
        if(w.ids.used || w.ids.other)Append(out,"ids",w.ids,false);
        if(w.flags.used || w.flags.other)Append(out,"flags",w.flags,false);
        out+="}";
    }
    // W4 FW-5 (R7, FX9): the scene light vector [[144961B50]+0x41D0]+0x10..+0x18 now, the same read in both modes.
    long long light_vector=-1;
    {uintptr_t root{},container{},first{},last{};
        if(image && read(image+0x4961B50,&root,8) && root && read(root+0x41D0,&container,8) && container &&
           read(container+0x10,&first,8) && read(container+0x18,&last,8) && last>=first)light_vector=static_cast<long long>((last-first)/8);}
    char tail[320]{};sprintf_s(tail,",\"nodes_last\":%u,\"nodes_max\":%u,\"update_before_collect\":%llu,\"collect_without_update\":%llu,\"light_vector\":%lld}\n",
        nodes_last,nodes_max,static_cast<unsigned long long>(update_first),static_cast<unsigned long long>(collect_alone),light_vector);out+=tail;
    log(out.c_str());
    for(auto& w:window){w.calls=0;w.system=0;w.callers.Clear();w.ids.Clear();w.flags.Clear();}
    nodes_max=0;update_first=0;collect_alone=0;row_frame=frames;
}
// The decoding of each entry's arguments (the prototypes of FFX-WAVE1-READY 1.2 and EFFECTS-ENTRY-DOOR-NATIVE 1).
void Observe(Entry entry,uintptr_t return_address,uint64_t a0,uint64_t a1,uint64_t a2,uint64_t a3,uintptr_t stack) noexcept {
    if(!enabled.load(std::memory_order_acquire) || entry>=EntryCount)return;
    bool has_id=false,has_flags=false,system=false,ok=false;uint32_t id{},flags{},nodes{};bool has_nodes=false;
    switch(entry){
    case SingleCreate:case MatrixCreate:case CutsceneCreate:{
        uint64_t effect_system{};system=a0 && read(a0+0x58,&effect_system,8) && effect_system;
        id=Dword(a2,ok);has_id=ok;
        if(stack){flags=Dword(stack,ok);has_flags=ok;}
        break;}
    case WorldRequest:case MatrixRequest:id=static_cast<uint32_t>(a1);flags=static_cast<uint32_t>(a3);has_id=has_flags=true;break;
    case BlockRequest:id=static_cast<uint32_t>(a2);has_id=true;break;
    case SfxUpdate:{uint64_t effect_system{};system=a0 && read(a0+0x58,&effect_system,8) && effect_system;break;}
    case NodeUpdate:nodes=Dword(a0?a0+0x30:0,ok);has_nodes=ok;break;
    default:break;
    }
    AcquireSRWLockExclusive(&lock);
    if(entry==GraphicsStep){++frames;if(frames<=3 || frames%600==0)Row();}
    auto& w=window[entry];++w.calls;if(system)++w.system;
    w.callers.Add(return_address>=image?return_address-image:return_address);
    if(has_id)w.ids.Add(id);
    if(has_flags)w.flags.Add(flags);
    if(has_nodes){nodes_last=nodes;if(nodes>nodes_max)nodes_max=nodes;}
    if(entry==SfxUpdate)update_sequence=++sequence;
    if(entry==LightCollect){(update_sequence>collect_sequence?update_first:collect_alone)+=1;collect_sequence=++sequence;}
    ReleaseSRWLockExclusive(&lock);
}
// Called by every thunk; `saved` is the thunk's block, the caller's return address sits 0x88 bytes above it.
void Record(uint32_t entry,const Saved* saved) noexcept {
    const auto entry_rsp=reinterpret_cast<uintptr_t>(saved)+0x88;
    Observe(static_cast<Entry>(entry),*reinterpret_cast<const uintptr_t*>(entry_rsp),saved->rcx,saved->rdx,saved->r8,saved->r9,entry_rsp+0x28);
}
// One executable page: a shared unwind record and function table, then one 256-byte thunk per entry.
constexpr size_t thunk_base=0x100,thunk_stride=0x100,page_size=thunk_base+thunk_stride*EntryCount;
void BuildPage() noexcept {
    auto* bytes=static_cast<BYTE*>(VirtualAlloc(nullptr,page_size,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    if(!bytes)return;
    std::memset(bytes,0xCC,page_size);
    // UNWIND_INFO: version 1, prolog 7 bytes, two codes: UWOP_ALLOC_LARGE at offset 7, size 0xA8/8.
    constexpr BYTE unwind[]{0x01,0x07,0x02,0x00,0x07,0x01,0x15,0x00};
    std::memcpy(bytes,unwind,sizeof(unwind));
    auto* table=reinterpret_cast<RUNTIME_FUNCTION*>(bytes+0x10);
    static_assert(0x10+sizeof(RUNTIME_FUNCTION)*EntryCount<=thunk_base);
    for(uint32_t e=0;e<EntryCount;++e){
        BYTE* t=bytes+thunk_base+thunk_stride*e;size_t n=0;
        auto put=[&](std::initializer_list<BYTE> code){for(auto b:code)t[n++]=b;};
        auto put64=[&](uint64_t v){std::memcpy(t+n,&v,8);n+=8;};
        put({0x48,0x81,0xEC,0xA8,0x00,0x00,0x00});                       // sub rsp,0xA8
        put({0x48,0x89,0x4C,0x24,0x20});put({0x48,0x89,0x54,0x24,0x28}); // mov [rsp+20],rcx; mov [rsp+28],rdx
        put({0x4C,0x89,0x44,0x24,0x30});put({0x4C,0x89,0x4C,0x24,0x38}); // mov [rsp+30],r8;  mov [rsp+38],r9
        put({0xF3,0x0F,0x7F,0x44,0x24,0x40});put({0xF3,0x0F,0x7F,0x4C,0x24,0x50});   // movdqu [rsp+40],xmm0; [rsp+50],xmm1
        put({0xF3,0x0F,0x7F,0x54,0x24,0x60});put({0xF3,0x0F,0x7F,0x5C,0x24,0x70});   // [rsp+60],xmm2; [rsp+70],xmm3
        put({0xF3,0x0F,0x7F,0xA4,0x24,0x80,0x00,0x00,0x00});put({0xF3,0x0F,0x7F,0xAC,0x24,0x90,0x00,0x00,0x00});   // xmm4, xmm5
        put({0xB9});std::memcpy(t+n,&e,4);n+=4;                            // mov ecx,entry
        put({0x48,0x8D,0x54,0x24,0x20});                                   // lea rdx,[rsp+20]
        put({0x48,0xB8});put64(reinterpret_cast<uint64_t>(&Record));       // mov rax,Record
        put({0xFF,0xD0});                                                  // call rax
        put({0xF3,0x0F,0x6F,0x44,0x24,0x40});put({0xF3,0x0F,0x6F,0x4C,0x24,0x50});
        put({0xF3,0x0F,0x6F,0x54,0x24,0x60});put({0xF3,0x0F,0x6F,0x5C,0x24,0x70});
        put({0xF3,0x0F,0x6F,0xA4,0x24,0x80,0x00,0x00,0x00});put({0xF3,0x0F,0x6F,0xAC,0x24,0x90,0x00,0x00,0x00});
        put({0x48,0x8B,0x4C,0x24,0x20});put({0x48,0x8B,0x54,0x24,0x28});
        put({0x4C,0x8B,0x44,0x24,0x30});put({0x4C,0x8B,0x4C,0x24,0x38});
        put({0x48,0x81,0xC4,0xA8,0x00,0x00,0x00});                       // add rsp,0xA8
        put({0x48,0xB8});put64(reinterpret_cast<uint64_t>(&originals[e])); // mov rax,&original
        put({0xFF,0x20});                                                  // jmp [rax]
        table[e].BeginAddress=static_cast<DWORD>(thunk_base+thunk_stride*e);
        table[e].EndAddress=static_cast<DWORD>(thunk_base+thunk_stride*e+n);
        table[e].UnwindData=0;
    }
    DWORD previous{};
    if(!VirtualProtect(bytes,page_size,PAGE_EXECUTE_READ,&previous)){VirtualFree(bytes,0,MEM_RELEASE);return;}
    FlushInstructionCache(GetCurrentProcess(),bytes,page_size);
    if(!RtlAddFunctionTable(table,EntryCount,reinterpret_cast<DWORD64>(bytes))){VirtualFree(bytes,0,MEM_RELEASE);return;}
    page=bytes;
}
}
void Configure(uintptr_t base,ReadFn reader,LogFn logger,const char* name) noexcept {
    image=base;read=reader;log=logger;mode=name;enabled.store(reader && logger,std::memory_order_release);
}
void* Thunk(Entry entry) noexcept {
    std::call_once(page_once,BuildPage);
    return page && entry<EntryCount?page+thunk_base+thunk_stride*entry:nullptr;
}
void** Original(Entry entry) noexcept {return entry<EntryCount?&originals[entry]:nullptr;}
void Note(Entry entry,uintptr_t return_address,uint64_t a0,uint64_t a1,uint64_t a2,uint64_t a3) noexcept {
    Observe(entry,return_address,a0,a1,a2,a3,0);
}
Window Snapshot(Entry entry) noexcept {
    AcquireSRWLockShared(&lock);Window copy=entry<EntryCount?window[entry]:Window{};ReleaseSRWLockShared(&lock);return copy;
}
}
