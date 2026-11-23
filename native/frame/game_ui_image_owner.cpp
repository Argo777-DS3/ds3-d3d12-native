#include "game_ui_image_owner.h"
#include "../content/acquire.h"   // C7-0 rule: in-process field reads through the installed guarded copy
#include <array>
#include <cstring>
#include <new>
#include <stdexcept>

namespace ds3native::bootstrap {
namespace {
template<class T>T Load(uintptr_t p){T value{};
    if(!p || !ds3content::Read(p,&value,sizeof(value)))
        throw std::runtime_error("native UI CPU field unreadable");return value;
}
template<class T>void Put(uintptr_t p,T value){std::memcpy(reinterpret_cast<void*>(p),&value,sizeof(value));}
template<class F>F Entry(uintptr_t module,uint32_t rva){return reinterpret_cast<F>(module+rva);}
struct CpuPlane {uint32_t width{},height{};uint64_t pitch{},bytes{};uintptr_t data{};};
struct CpuImageData {uint32_t format{},use{};uint8_t flags{},mips{};uint16_t planes{};uint32_t pad{};uintptr_t table{},palette{};CpuPlane inline_plane;};
struct CpuUpdate {CpuPlane source;std::array<int32_t,4> destination;uint32_t plane{},pad{};};
static_assert(sizeof(CpuPlane)==32 && sizeof(CpuImageData)==64 && sizeof(CpuUpdate)==56);
struct CpuImage {
    std::array<uint8_t,0xA8> prefix{};
    std::shared_ptr<GameUiImageOwner::State> owner;
    std::array<uint64_t,4> imported_format_row{};
    std::array<uintptr_t,3> imported_format_wrapper{};
    bool imported{};
};
struct CpuManager {
    std::array<uint8_t,0xC0> prefix{};
    std::shared_ptr<GameUiImageOwner::State> owner;
};
static_assert(offsetof(CpuImage,owner)==0xA8 && offsetof(CpuManager,owner)==0xC0);
using CpuPhase=void(__fastcall*)(uintptr_t);
using CpuRelease=void(__fastcall*)(uintptr_t);
struct CurrentImageBindingScope {
    GameUiImageOwner::State* owner{};
    UiDrawImageSource source;
};
thread_local CurrentImageBindingScope* current_image_binding_scope{};
}
struct GameUiImageOwner::State {
    uintptr_t module{};
    UiImageIngress images;
    DeviceQueues& gpu;
    std::mutex& submissions;
    Failure failure{};
    std::recursive_mutex mutex;
    bool closed{},r8_supported{},rgba_supported{};
    uintptr_t manager{};
    std::array<uintptr_t,29> manager_table{};
    std::array<uintptr_t,3> secondary_table{};
    std::array<uintptr_t,24> image_table{};
    std::array<uintptr_t,4> format_table{};
    std::array<std::array<uintptr_t,3>,2> format_wrappers{};
    std::array<std::array<uint64_t,4>,2> format_rows{};
    State(uintptr_t m,TextureIngress& u,DeviceQueues& g,std::mutex& s,uint64_t incarnation,Failure f):
        module(m),images(u,incarnation),gpu(g),submissions(s),failure(f){}
    [[noreturn]] void Fail(const char* reason) const noexcept {failure(reason);std::terminate();}
    uintptr_t Allocate(uintptr_t owner,size_t bytes){
        const auto heap=Load<uintptr_t>(module+0x4936CC0),table=Load<uintptr_t>(heap);
        const uint32_t hint=0x48;
        if(!owner){using RootAllocate=uintptr_t(__fastcall*)(uintptr_t,size_t,const uint32_t*);
            const auto p=reinterpret_cast<RootAllocate>(Load<uintptr_t>(table+0x50))(heap,bytes,&hint);
            if(!p)throw std::bad_alloc();return p;
        }
        using Allocate=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,size_t,const uint32_t*);
        const auto p=reinterpret_cast<Allocate>(Load<uintptr_t>(table+0x70))(heap,owner,bytes,&hint);
        if(!p)throw std::bad_alloc();return p;
    }
    void Free(uintptr_t p){const auto heap=Load<uintptr_t>(module+0x4936CC0);
        using Free=void(__fastcall*)(uintptr_t,uintptr_t);
        reinterpret_cast<Free>(Load<uintptr_t>(Load<uintptr_t>(heap)+0x60))(heap,p);
    }
    void Release(uintptr_t p) const {if(p)Entry<CpuRelease>(module,0x1BB6D00)(p);}
    void Retain(uintptr_t p) const {Entry<int(__fastcall*)(uintptr_t)>(module,0x1BB6C10)(p);}
};
namespace {
auto Owner(uintptr_t p,bool manager){return manager?reinterpret_cast<CpuManager*>(p)->owner:reinterpret_cast<CpuImage*>(p)->owner;}
[[noreturn]] void __fastcall UnsupportedManager(uintptr_t p){Owner(p,true)->Fail("unqualified_native_ui_manager_method");}
[[noreturn]] void __fastcall UnsupportedImage(uintptr_t p){Owner(p,false)->Fail("unqualified_native_ui_image_method");}
uintptr_t __fastcall OptionalNone(uintptr_t){return 0;}
uint32_t __fastcall Format(uintptr_t p){return Load<uint32_t>(Load<uintptr_t>(p+8));}
uintptr_t __fastcall CopyFunction(uintptr_t p){return Load<uintptr_t>(Load<uintptr_t>(p+8)+16);}
uintptr_t __fastcall InverseCopyFunction(uintptr_t p){return Load<uintptr_t>(Load<uintptr_t>(p+8)+24);}
[[noreturn]] void __fastcall UnsupportedFormat(uintptr_t p){reinterpret_cast<GameUiImageOwner::State*>(Load<uintptr_t>(p+16))->Fail("native_ui_borrowed_format_retirement");}
uint32_t __fastcall Capabilities(uintptr_t p,uint32_t format){const auto state=Owner(p,true);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    // Only witnessed CPU raster updates are advertised. Mapping and unrelated
    // usage families are absent; residency is established by Create/Update.
    return !state->closed && state->r8_supported && format==9?0x20u:0u;
}
uintptr_t __fastcall LookupFormat(uintptr_t p,uint32_t format){const auto state=Owner(p,true);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed)return 0;
    if(state->r8_supported && format==9)return reinterpret_cast<uintptr_t>(state->format_wrappers[0].data());
    if(state->rgba_supported && format==1)return reinterpret_cast<uintptr_t>(state->format_wrappers[1].data());
    return 0;
}
bool __fastcall ManagerAvailable(uintptr_t p){const auto state=Owner(p,true);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);return !state->closed;
}
void __fastcall DetachImageSource(uintptr_t p){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);Entry<CpuPhase>(state->module,0x1F83B10)(p);
}
void __fastcall LoseImageManager(uintptr_t p){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);Entry<CpuPhase>(state->module,0x1BD0990)(p);
}
void __fastcall RecoverImage(uintptr_t p){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);Entry<CpuPhase>(state->module,0x1BD0A30)(p);
}
void __fastcall BindImage(uintptr_t p,uint32_t slot,uintptr_t sampler){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    auto* scope=current_image_binding_scope;
    if(state->closed || !scope || scope->owner!=state.get() || Load<uint32_t>(p+0x40)!=2 || Load<uint8_t>(p+0x45)!=1 ||
       Load<uintptr_t>(state->manager+0x40) || Load<uintptr_t>(state->manager+0x48) ||
       !scope->source.Bind(state->images.Acquire(),p,slot,Load<uint8_t>(sampler)))state->Fail("native_ui_current_image_bind_contract");
}
struct CpuImageLock {
    uintptr_t module{},address{};
    CpuImageLock(uintptr_t m,uintptr_t image):module(m),address(Load<uintptr_t>(image+0x28)+0x18){Entry<CpuPhase>(module,0x1BCB340)(address);}
    ~CpuImageLock(){Entry<CpuPhase>(module,0x1BCB370)(address);}
};
void __fastcall RetireImageGpu(uintptr_t p,bool){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    const auto current=state->images.Acquire();
    if(current && current->images.contains(p) && !state->images.Retire(p,current->version))state->Fail("native_ui_retire_generation");
}
bool __fastcall ImageValid(uintptr_t p){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    // The original CPU queue checks this structural predicate BEFORE Init.
    // Pending is valid allocated CPU metadata, not GPU residency/readiness.
    if(!state->closed && Load<uint32_t>(p+0x40)==0)
        return Load<uintptr_t>(p+0x60)==p+0x68 && Load<uint8_t>(p+0x45)==1;
    const auto current=state->images.Acquire();if(!current)return false;
    const auto found=current->images.find(p);
    return found!=current->images.end() && found->second.gpu && found->second.gpu->resident;
}
bool DecodeAndPublishImage(uintptr_t p,bool initial){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    // Failed native admission must reach the real CPU queue's failure state.
    if(initial)Put<uint32_t>(p+0x40,1);
    if(state->closed)return false;
    try {
        if(reinterpret_cast<CpuImage*>(p)->imported){
            const auto current=state->images.Acquire();if(!current)return false;
            const auto found=current->images.find(p);
            if(found==current->images.end() || !found->second.gpu || !found->second.gpu->resident)return false;
            Put<uint32_t>(p+0x40,2);
            if(initial)Entry<uintptr_t(__fastcall*)(uintptr_t)>(state->module,0x1BD08C0)(p);
            return true;
        }
        const auto width=Load<uint32_t>(p+0x38),height=Load<uint32_t>(p+0x3C);
        const auto format=Format(Load<uintptr_t>(p+0x58));
        if(!width || !height || width>8192 || height>8192 ||
           (format!=9 && format!=1) || uint64_t(width)*height*(format==1?4:1)>(8u<<20))return false;
        std::vector<uint8_t> pixels;
        uint64_t pitch=width;
        if(format==9){
            if(!state->r8_supported)return false;
            pixels.resize(size_t(width)*height);
        }else{
            if(!state->rgba_supported)return false;
            // This family is the actual pure CPU gradient decoder. Preserve
            // its source/allocator and copy callback, replace GPU upload only.
            const auto source=Load<uintptr_t>(p+0x30),table=Load<uintptr_t>(source);
            if(Load<uintptr_t>(table+0x40)!=state->module+0x1CEEC30 ||
               Load<uintptr_t>(table+0x18)!=state->module+0x1CEF1A0 ||
               Load<uint32_t>(source+0x38)!=width || Load<uint32_t>(source+0x3C)!=height || width>256 || height>256)return false;
            state->Retain(source);
            const std::shared_ptr<void> source_owner(reinterpret_cast<void*>(source),[state](void* q){state->Release(reinterpret_cast<uintptr_t>(q));});
            const std::array<uint32_t,2> size{width,height};
            using RawFactory=uintptr_t(__fastcall*)(uint32_t,int,const uint32_t*,uint32_t,uintptr_t,uintptr_t);
            const auto raw=Entry<RawFactory>(state->module,0x1BCE9C0)(1,1,size.data(),0,0,0);
            if(!raw)return false;
            const std::shared_ptr<void> raw_owner(reinterpret_cast<void*>(raw),[state](void* q){state->Release(reinterpret_cast<uintptr_t>(q));});
            if(Load<uintptr_t>(raw)!=state->module+0x3AD5E80 || Load<uintptr_t>(raw+0x10))return false;
            auto data=Load<CpuImageData>(raw+0x28);
            if(data.format!=1 || data.mips!=1 || data.planes!=1 || !data.table)return false;
            const auto allocation=Load<CpuPlane>(data.table);
            if(allocation.width!=width || allocation.height!=height || allocation.pitch<uint64_t(width)*4 ||
               allocation.pitch>(8u<<20) || allocation.pitch*height>allocation.bytes ||
               allocation.bytes>(8u<<20) || !allocation.data || allocation.bytes>UINTPTR_MAX-allocation.data)return false;
            // Original1BD1310 sets this bit on its local CPU decode descriptor;
            // the owned RawImage keeps its original format/allocation metadata.
            data.format|=0x100000;
            using Decode=bool(__fastcall*)(uintptr_t,CpuImageData*,uintptr_t,uintptr_t);
            if(!Entry<Decode>(state->module,0x1CEEC30)(source,&data,state->module+0x1BCE930,0))return false;
            pitch=allocation.pitch;pixels.resize(static_cast<size_t>(pitch*height));SIZE_T read{};
            if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(allocation.data),pixels.data(),pixels.size(),&read) || read!=pixels.size())return false;
        }
        const std::array<CpuPrimitiveImageMip,1> input{{{width,height,pitch,pixels}}};
        const auto current=state->images.Acquire();
        if(!current || !state->images.Create(p,current->version,format,input))return false;
        // Original getters use physical plane extent and source-declared size.
        // No legacy resource/view/sampler is written into the remaining bytes.
        Put<uint32_t>(p+0x68,width);Put<uint32_t>(p+0x6C,height);
        Put<uint32_t>(p+0x40,2);
        if(initial)Entry<uintptr_t(__fastcall*)(uintptr_t)>(state->module,0x1BD08C0)(p);return true;
    }catch(...){if(initial)Put<uint32_t>(p+0x40,1);return false;}
}
bool __fastcall InitializeImage(uintptr_t p){return DecodeAndPublishImage(p,true);}
void __fastcall DecodeCurrentImage(uintptr_t p){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed || Load<uint32_t>(p+0x40)!=2)state->Fail("native_ui_command_image_not_resident");
    if(reinterpret_cast<CpuImage*>(p)->imported){if(!ImageValid(p))state->Fail("native_ui_imported_image_lost");return;}
    const CpuImageLock cpu_lock(state->module,p);
    const auto source=Load<uintptr_t>(p+0x30),table=Load<uintptr_t>(source);
    if(table==state->module+0x3AE0FE0){
        // The genuine glyph Source has no full-image decoder (E48260=false).
        // Whole-decode commands preserve the existing raster atlas; actual
        // current rectangles arrive separately through native image +98.
        if(Load<uintptr_t>(table+0x40)!=state->module+0xE48260 ||
           Entry<bool(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uintptr_t)>(state->module,0xE48260)(source,0,0,0))
            state->Fail("native_ui_glyph_full_decoder_contract");
        return;
    }
    if(table!=state->module+0x3AE4C90 || !DecodeAndPublishImage(p,false))state->Fail("native_ui_current_cpu_decode_failed");
}
uintptr_t __fastcall DestroyImage(uintptr_t p,uint32_t flags){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    try{
        const auto helper=Load<uintptr_t>(p+0x28);
        Entry<CpuPhase>(state->module,0x1BB4880)(helper+0x48);
        const auto previous=Load<uintptr_t>(p+0x10),next=Load<uintptr_t>(p+0x18);
        if(previous!=UINTPTR_MAX && next!=UINTPTR_MAX){
            Put<uintptr_t>(previous+0x18,next);Put<uintptr_t>(next+0x10,previous);
            Put<uintptr_t>(p+0x10,UINTPTR_MAX);Put<uintptr_t>(p+0x18,UINTPTR_MAX);
        }
        RetireImageGpu(p,true);
        if(Load<uintptr_t>(p+0x20) || Load<uintptr_t>(p+0x60)!=p+0x68)state->Fail("native_ui_image_cpu_auxiliary_contract");
        Entry<CpuPhase>(state->module,0x1BB4FA0)(helper+0x48);
        state->Release(helper);
        reinterpret_cast<CpuImage*>(p)->owner.~shared_ptr();
        if(flags&1)state->Free(p);return p;
    }catch(...){state->Fail("native_ui_image_cpu_retirement_failed");}
}
bool __fastcall UpdateImage(uintptr_t p,uintptr_t records,uint32_t count,uint32_t mip){const auto state=Owner(p,false);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed || !count || count>4096 || mip || Load<uint32_t>(p+0x40)!=2)return false;
    try{
        const CpuImageLock cpu_lock(state->module,p);
        const auto source=Load<uintptr_t>(p+0x30);
        if(Load<uintptr_t>(source)!=state->module+0x3AE0FE0)return false;
        const auto cache=Load<uintptr_t>(source+0x38);
        if(Load<uint32_t>(cache+0xFE0)!=1)return false;
        const auto raw=Load<uintptr_t>(cache+0xFF8);
        if(Load<uintptr_t>(raw)!=state->module+0x3AD5E80 || Load<uintptr_t>(raw+0x10))return false;
        state->Retain(raw);
        const std::shared_ptr<void> retained(reinterpret_cast<void*>(raw),[state](void* q){state->Release(reinterpret_cast<uintptr_t>(q));});
        const auto data=Load<CpuImageData>(raw+0x28);
        if((data.format&0xFFF)!=9 || data.mips!=1 || data.planes!=1)return false;
        const auto allocation=Load<CpuPlane>(data.table);
        if(!allocation.data || !allocation.bytes || allocation.bytes>(8u<<20) || allocation.bytes>UINTPTR_MAX-allocation.data)return false;
        std::vector<std::vector<uint8_t>> copies(count);std::vector<CpuGlyphImageUpdate> updates;updates.reserve(count);
        uint64_t copied{};
        for(uint32_t ordinal=0;ordinal<count;++ordinal){const auto record=Load<CpuUpdate>(records+size_t(ordinal)*56);
            const auto& rect=record.destination;
            if(record.plane || rect[0]<0 || rect[1]<0 || rect[2]<=rect[0] || rect[3]<=rect[1] ||
               record.source.width!=allocation.width || record.source.height!=allocation.height || record.source.pitch!=allocation.pitch ||
               record.source.data<allocation.data)return false;
            const auto width=uint32_t(rect[2]-rect[0]),height=uint32_t(rect[3]-rect[1]);
            if(allocation.pitch<width || allocation.pitch>(8u<<20) || uint64_t(height-1)*allocation.pitch+width>(8u<<20))return false;
            const auto bytes=uint64_t(height-1)*allocation.pitch+width,offset=record.source.data-allocation.data;
            if(offset>allocation.bytes || bytes>allocation.bytes-offset || bytes>(256u<<20)-copied)return false;
            copied+=bytes;auto& copy=copies[ordinal];copy.resize(static_cast<size_t>(bytes));SIZE_T read{};
            if(!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(record.source.data),copy.data(),copy.size(),&read) || read!=copy.size())return false;
            updates.push_back({{record.source.width,record.source.height,record.source.pitch,copy},rect,0});
        }
        const auto current=state->images.Acquire();return current && state->images.UpdateGlyphs(p,current->version,updates);
    }catch(...){return false;}
}
void __fastcall NativeRetirementPump(uintptr_t){/* Native upload/retirement is synchronous and fenced. */}
void __fastcall PendingCpuImages(uintptr_t p){const auto state=Owner(p,true);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);Entry<CpuPhase>(state->module,0x1F851D0)(p);
}
struct CpuManagerLock {
    uintptr_t module{},address{};
    CpuManagerLock(uintptr_t m,uintptr_t manager):module(m),address(Load<uintptr_t>(manager+0x58)+0x48){Entry<CpuPhase>(module,0x1BB4880)(address);}
    ~CpuManagerLock(){Entry<CpuPhase>(module,0x1BB4FA0)(address);}
};
void ValidateCommandSource(const std::shared_ptr<GameUiImageOwner::State>& state,uintptr_t source){
    const auto table=Load<uintptr_t>(source);
    if(table!=state->module+0x3AE0FE0 && table!=state->module+0x3AE4C90)state->Fail("native_ui_unqualified_command_source");
    const auto association=Load<uintptr_t>(source+0x10);
    const auto current=state->images.Acquire();
    // A source associated with another backend must never enter its getter:
    // gradient GetTexture would clear that association when managers differ.
    if(association && (!current || !current->images.contains(association)))state->Fail("native_ui_foreign_command_association");
}
void __fastcall QueueSourceDecode(uintptr_t secondary,uintptr_t source){const auto manager=secondary-0x10;const auto state=Owner(manager,true);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed)state->Fail("native_ui_command_after_close");ValidateCommandSource(state,source);
    using Queue=void(__fastcall*)(uintptr_t,uintptr_t);
    Entry<Queue>(state->module,0x1BD1C00)(secondary,source);
}
[[noreturn]] void __fastcall QueueUnsupportedWork(uintptr_t secondary){Owner(secondary-0x10,true)->Fail("native_ui_unqualified_plain_cpu_command");}
void __fastcall ConsumeCpuCommands(uintptr_t p){const auto state=Owner(p,true);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed)state->Fail("native_ui_command_consumption_after_close");
    const CpuManagerLock cpu_lock(state->module,p);
    NativeRetirementPump(p);PendingCpuImages(p);
    const auto table=Load<uintptr_t>(p+0x60),count=Load<uint64_t>(p+0x68),capacity=Load<uint64_t>(p+0x70)&~uint64_t{1};
    if(count>4096 || count>capacity || (count && !table))state->Fail("native_ui_cpu_command_array_contract");
    for(uint64_t ordinal=0;ordinal<count;++ordinal){const auto command=Load<uintptr_t>(table+size_t(ordinal)*8);
        if(!(command&1))state->Fail("native_ui_unqualified_plain_cpu_command");
        ValidateCommandSource(state,command&~uintptr_t{1});
    }
    // Qualified CPU consumer preserves actual Source getter/ref semantics.
    // Every dispatched image +90 is native; arbitrary command v8 is refused
    // before this call. Resize/free uses the genuine CPU engine heap.
    using Consume=void(__fastcall*)(uintptr_t,uintptr_t);
    Entry<Consume>(state->module,0x1BD0D10)(p+0x60,p);
}
void __fastcall WithdrawManager(uintptr_t p){const auto state=Owner(p,true);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    // Preserve genuine CPU list removal/source TextureLost notifications via
    // the base image method, whose +60 GPU retirement callback is native.
    for(;;){const auto tail=Load<uintptr_t>(p+0x98);if(tail==p+0x80)break;
        LoseImageManager(tail);
    }
}
uintptr_t __fastcall DestroyManager(uintptr_t p,uint32_t flags){const auto state=Owner(p,true);
    const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    try{
        if(!state->closed)ConsumeCpuCommands(p);
        state->closed=true;WithdrawManager(p);state->images.Close();
        const auto helper=Load<uintptr_t>(p+0x58);Put<uintptr_t>(helper+0x10,0);state->manager=0;
        // All optional CPU callback/format/command arrays are unused by this
        // narrow synchronous owner. Refuse instead of dropping unseen work.
        for(const auto offset:{0x38u,0x40u,0x48u,0x50u,0x68u,0x78u})
            if(Load<uintptr_t>(p+offset))state->Fail("native_ui_manager_cpu_retirement_contract");
        const auto commands=Load<uintptr_t>(p+0x60);if(commands)state->Free(commands);
        Put<uintptr_t>(p+0x60,0);Put<uint64_t>(p+0x70,0);
        state->Release(helper);
        reinterpret_cast<CpuManager*>(p)->owner.~shared_ptr();
        if(flags&1)state->Free(p);return p;
    }catch(...){state->Fail("native_ui_manager_cpu_retirement_failed");}
}
uintptr_t __fastcall DestroySecondary(uintptr_t p,uint32_t flags){return DestroyManager(p-0x10,flags);}
uintptr_t __fastcall CreateImage(uintptr_t manager,uint32_t format,uint32_t mips,uintptr_t size,uint32_t use,uintptr_t source,uintptr_t){
    const auto state=Owner(manager,true);const std::lock_guard<std::recursive_mutex> hold(state->mutex);
    if(state->closed || mips!=1 ||
       !((state->r8_supported && format==9 && use==0x20) || (state->rgba_supported && format==1 && use==1)))return 0;
    try{
        const auto width=Load<uint32_t>(size),height=Load<uint32_t>(size+4);
        if(!width || !height || width>8192 || height>8192 || uint64_t(width)*height*(format==1?4:1)>(8u<<20))return 0;
        if(format==9){
            if(Load<uintptr_t>(source)!=state->module+0x3AE0FE0 || Load<uint32_t>(source+0x28)!=9 ||
               Load<uint32_t>(source+0x2C)!=width || Load<uint32_t>(source+0x30)!=height || Load<uint32_t>(source+0x34)!=use)return 0;
        }else{
            const auto table=Load<uintptr_t>(source);
            if(Load<uintptr_t>(table+0x40)!=state->module+0x1CEEC30 || Load<uintptr_t>(table+0x18)!=state->module+0x1CEF1A0 ||
               Load<uint32_t>(source+0x38)!=width || Load<uint32_t>(source+0x3C)!=height || width>256 || height>256)return 0;
        }
        const auto p=state->Allocate(manager,sizeof(CpuImage));
        new(reinterpret_cast<void*>(p)) CpuImage{};
        // The qualified original CPU constructor retains the actual lock
        // helper and initializes real ref/list/source/format/plane metadata.
        using Construct=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uintptr_t,uint16_t,uintptr_t);
        Entry<Construct>(state->module,0x1F82940)(p,Load<uintptr_t>(manager+0x58),LookupFormat(manager,format),mips,size,static_cast<uint16_t>(use),source);
        reinterpret_cast<CpuImage*>(p)->owner=state;
        Put<uintptr_t>(p,reinterpret_cast<uintptr_t>(state->image_table.data()));
        // CPU queue/list transaction invokes only our native Init/retirement
        // and the truthful zero-thread CPU thread policy. No old GPU Map.
        using Publish=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uint32_t);
        return Entry<Publish>(state->module,0x1BD20F0)(manager,p,use);
    }catch(...){return 0;}
}
void BuildTables(GameUiImageOwner::State& state){
    state.manager_table.fill(reinterpret_cast<uintptr_t>(&UnsupportedManager));
    state.image_table.fill(reinterpret_cast<uintptr_t>(&UnsupportedImage));
    auto manager=[&](size_t slot,uintptr_t fn){state.manager_table[slot/8]=fn;};
    auto image=[&](size_t slot,uintptr_t fn){state.image_table[slot/8]=fn;};
    manager(0,reinterpret_cast<uintptr_t>(&DestroyManager));manager(8,reinterpret_cast<uintptr_t>(&OptionalNone));
    manager(0x10,reinterpret_cast<uintptr_t>(&CreateImage));manager(0x18,reinterpret_cast<uintptr_t>(&OptionalNone));
    manager(0x20,reinterpret_cast<uintptr_t>(&Capabilities));manager(0x28,reinterpret_cast<uintptr_t>(&ManagerAvailable));
    manager(0x38,reinterpret_cast<uintptr_t>(&OptionalNone));manager(0x40,state.module+0x1BCE850);
    manager(0x48,reinterpret_cast<uintptr_t>(&ConsumeCpuCommands));
    manager(0x50,reinterpret_cast<uintptr_t>(&WithdrawManager));manager(0x90,reinterpret_cast<uintptr_t>(&NativeRetirementPump));
    manager(0x98,reinterpret_cast<uintptr_t>(&PendingCpuImages));manager(0xA0,reinterpret_cast<uintptr_t>(&LookupFormat));
    for(const auto slot:{0xA8u,0xB0u,0xB8u,0xC8u,0xD0u})manager(slot,reinterpret_cast<uintptr_t>(&OptionalNone));
    state.secondary_table={reinterpret_cast<uintptr_t>(&DestroySecondary),reinterpret_cast<uintptr_t>(&QueueUnsupportedWork),reinterpret_cast<uintptr_t>(&QueueSourceDecode)};
    image(0,reinterpret_cast<uintptr_t>(&DestroyImage));image(8,reinterpret_cast<uintptr_t>(&InitializeImage));
    image(0x10,reinterpret_cast<uintptr_t>(&ImageValid));
    for(const auto [slot,rva]:std::array<std::pair<size_t,uint32_t>,6>{{{0x18,0x1F83990},{0x20,0x1F83980},{0x28,0x1F839A0},{0x30,0x1F83A30},{0x38,0x1F83AC0},{0xA0,0x1BCF5B0}}})image(slot,state.module+rva);
    image(0x40,reinterpret_cast<uintptr_t>(&DetachImageSource));image(0x48,reinterpret_cast<uintptr_t>(&LoseImageManager));image(0x50,reinterpret_cast<uintptr_t>(&RecoverImage));
    image(0x60,reinterpret_cast<uintptr_t>(&RetireImageGpu));image(0x68,reinterpret_cast<uintptr_t>(&BindImage));
    image(0x90,reinterpret_cast<uintptr_t>(&DecodeCurrentImage));image(0x98,reinterpret_cast<uintptr_t>(&UpdateImage));
    state.format_table={reinterpret_cast<uintptr_t>(&UnsupportedFormat),reinterpret_cast<uintptr_t>(&Format),reinterpret_cast<uintptr_t>(&CopyFunction),reinterpret_cast<uintptr_t>(&InverseCopyFunction)};
    state.format_rows[0]={uint64_t(DXGI_FORMAT_R8_UNORM)<<32|9u,1u,state.module+0x1BCE930,state.module+0x1BCE930};
    state.format_rows[1]={uint64_t(DXGI_FORMAT_R8G8B8A8_UNORM)<<32|1u,4u,state.module+0x1BCE930,state.module+0x1BCE930};
    for(size_t ordinal=0;ordinal<state.format_wrappers.size();++ordinal)
        state.format_wrappers[ordinal]={reinterpret_cast<uintptr_t>(state.format_table.data()),reinterpret_cast<uintptr_t>(state.format_rows[ordinal].data()),reinterpret_cast<uintptr_t>(&state)};
}
}
GameUiImageOwner::GameUiImageOwner(uintptr_t module,TextureIngress& uploads,DeviceQueues& gpu,std::mutex& submissions,uint64_t incarnation,Failure failure){
    if(!module || !failure)throw std::invalid_argument("native UI CPU owner services");
    state_=std::make_shared<State>(module,uploads,gpu,submissions,incarnation,failure);BuildTables(*state_);
    const std::lock_guard<std::mutex> submit(submissions);
    for(const auto [format,admitted]:std::array<std::pair<DXGI_FORMAT,bool*>,2>{{{DXGI_FORMAT_R8_UNORM,&state_->r8_supported},{DXGI_FORMAT_R8G8B8A8_UNORM,&state_->rgba_supported}}}){
        D3D12_FEATURE_DATA_FORMAT_SUPPORT support{format};
        *admitted=gpu.device && SUCCEEDED(gpu.device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT,&support,sizeof(support))) &&
            (support.Support1&(D3D12_FORMAT_SUPPORT1_TEXTURE2D|D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE))==(D3D12_FORMAT_SUPPORT1_TEXTURE2D|D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE);
    }
}
GameUiImageOwner::~GameUiImageOwner(){Close();if(manager_)state_->Release(reinterpret_cast<uintptr_t>(manager_));}
// I-0120 G9: no manager (the owner closed, or no R8 sampling) means no CPU command pending: nothing to consume.
void GameUiImageOwner::ConsumeCpuPending(){if(const auto manager=reinterpret_cast<uintptr_t>(ConstructManager()))ConsumeCpuCommands(manager);}
void* GameUiImageOwner::ConstructManager(){const std::lock_guard<std::recursive_mutex> hold(state_->mutex);
    if(state_->closed || !state_->r8_supported)return nullptr;if(manager_)return manager_;
    const auto p=state_->Allocate(0,sizeof(CpuManager));
    new(reinterpret_cast<void*>(p)) CpuManager{};
    using Construct=uintptr_t(__fastcall*)(uintptr_t,uint32_t,uintptr_t,uintptr_t);
    Entry<Construct>(state_->module,0x1BCDED0)(p,0,0,0);
    reinterpret_cast<CpuManager*>(p)->owner=state_;
    if(!Load<uintptr_t>(p+0x58)){reinterpret_cast<CpuManager*>(p)->owner.~shared_ptr();state_->Free(p);throw std::bad_alloc();}
    Put<uintptr_t>(p,reinterpret_cast<uintptr_t>(state_->manager_table.data()));
    Put<uintptr_t>(p+0x10,reinterpret_cast<uintptr_t>(state_->secondary_table.data()));
    state_->manager=p;manager_=reinterpret_cast<void*>(p);return manager_;
}
UiImageIngress::Lease GameUiImageOwner::Acquire() const{return state_->images.Acquire();}
uintptr_t GameUiImageOwner::ImportTexture(std::shared_ptr<const NativeTextureRecord> texture,const char** not_taken){
    const auto why=[&](const char* reason){if(not_taken)*not_taken=reason;return uintptr_t{};};
    const auto manager=reinterpret_cast<uintptr_t>(ConstructManager());
    const std::lock_guard<std::recursive_mutex> hold(state_->mutex);
    if(!manager || state_->closed)return why("owner_closed");
    if(!texture || !texture->resident || !texture->content)return why("texture_not_resident");
    if(texture->description.cube || texture->description.slices!=1)return why("format");
    uint32_t format{},block_bytes{};
    // Same single-plane CPU format identities as the recovered46F8EB0 table.
    switch(texture->description.format){
    case DXGI_FORMAT_R8G8B8A8_UNORM:format=1;block_bytes=4;break;
    case DXGI_FORMAT_B8G8R8A8_UNORM:format=2;block_bytes=4;break;
    case DXGI_FORMAT_R8_UNORM:format=9;block_bytes=1;break;
    case DXGI_FORMAT_BC1_UNORM:format=50;block_bytes=8;break;
    case DXGI_FORMAT_BC2_UNORM:format=51;block_bytes=16;break;
    case DXGI_FORMAT_BC3_UNORM:format=52;block_bytes=16;break;
    case DXGI_FORMAT_BC7_UNORM:format=53;block_bytes=16;break;
    default:return why("format");
    }
    // A full set is said before anything is made (the final review's L4: each refused import built and released an image).
    {const auto held=state_->images.Acquire();if(held && held->textures && held->textures->textures.size()>=frame::PrimitiveTextureLimit)return why("set_full");}
    const auto p=state_->Allocate(manager,sizeof(CpuImage));
    auto* image=new(reinterpret_cast<void*>(p)) CpuImage{};image->owner=state_;image->imported=true;
    image->imported_format_row={uint64_t(texture->description.format)<<32|format,block_bytes,state_->module+0x1BCE930,state_->module+0x1BCE930};
    image->imported_format_wrapper={reinterpret_cast<uintptr_t>(state_->format_table.data()),reinterpret_cast<uintptr_t>(image->imported_format_row.data()),reinterpret_cast<uintptr_t>(state_.get())};
    const std::array<uint32_t,2> size{texture->description.width,texture->description.height};
    using Construct=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uintptr_t,uint32_t,uintptr_t,uint16_t,uintptr_t);
    Entry<Construct>(state_->module,0x1F82940)(p,Load<uintptr_t>(manager+0x58),reinterpret_cast<uintptr_t>(image->imported_format_wrapper.data()),texture->description.mips,reinterpret_cast<uintptr_t>(size.data()),0x100,0);
    Put<uintptr_t>(p,reinterpret_cast<uintptr_t>(state_->image_table.data()));
    Put<uint32_t>(p+0x68,size[0]);Put<uint32_t>(p+0x6C,size[1]);
    const auto current=state_->images.Acquire();
    if(!current || !state_->images.Import(p,current->version,std::move(texture))){state_->Release(p);
        return why(current && current->textures && current->textures->textures.size()>=frame::PrimitiveTextureLimit?"set_full":"set_refused");}
    using Publish=uintptr_t(__fastcall*)(uintptr_t,uintptr_t,uint32_t);
    const auto published=Entry<Publish>(state_->module,0x1BD20F0)(manager,p,0x100);
    if(!published)why("publish_refused");   // the engine's own publication answered 0: named for the caller's refusal
    return published;
}
bool GameUiImageOwner::PrepareBindings(uint64_t serial,uint64_t draw_identity,const std::function<void()>& prepare,UiDrawImageBinding& output){
    const std::lock_guard<std::recursive_mutex> hold(state_->mutex);
    if(state_->closed || current_image_binding_scope || !prepare)return false;
    CurrentImageBindingScope scope;scope.owner=state_.get();
    if(!scope.source.Begin(state_->images.Acquire(),serial,draw_identity))return false;
    struct Reset {~Reset(){current_image_binding_scope=nullptr;}} reset;
    current_image_binding_scope=&scope;prepare();
    const auto current=state_->images.Acquire();return current && scope.source.Resolve(current->version,serial,output);
}
void GameUiImageOwner::Close(){const std::lock_guard<std::recursive_mutex> hold(state_->mutex);
    if(state_->closed)return;
    if(state_->manager)ConsumeCpuCommands(state_->manager);state_->closed=true;
    if(state_->manager)WithdrawManager(state_->manager);state_->images.Close();
}
}
