#include "game_ui_session.h"
#include "../content/acquire.h"   // C7-0 rule: in-process field reads through the installed guarded copy
#include "game_ui_primitive.h"
#include "dark_logo.h"   // D-0182 (I-0135)
#include "dark_logo_kept.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <vector>
#include <optional>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstddef>
#include <memory>
#include <new>
#include <intrin.h>
#include <atomic>

namespace ds3native::bootstrap {
namespace {
// D-0167 measurement (GameUiSession::Profile): ticks of PrepareFrame, of the movies' display calls, of the draw
// submissions, of the final packet, then draws and frames; on measure runs only (SetProfiling).
std::atomic<bool> ui_profiling{};
std::atomic<bool> ui_cache{false};   // W1 F8a (SetUiCache): opt-in until its page (04_030) is witnessed
std::array<std::atomic<uint64_t>,10> ui_profile{};   // + 6: shader uniforms and image bindings, 7: primitive packets, 8/9: image bytes taken into draw packets / of them already in the frame
int64_t UiStamp(bool on) noexcept {LARGE_INTEGER now{};if(on)QueryPerformanceCounter(&now);return now.QuadPart;}
void UiAdd(size_t slot,int64_t since,bool on) noexcept {if(on)ui_profile[slot].fetch_add(uint64_t(UiStamp(true)-since),std::memory_order_relaxed);}
struct UiTimer {size_t slot{};int64_t start{};bool on{};
    explicit UiTimer(size_t which) noexcept:slot(which),on(ui_profiling.load(std::memory_order_relaxed)){
        if(on){LARGE_INTEGER now{};QueryPerformanceCounter(&now);start=now.QuadPart;}}
    ~UiTimer(){if(on){LARGE_INTEGER now{};QueryPerformanceCounter(&now);ui_profile[slot].fetch_add(uint64_t(now.QuadPart-start),std::memory_order_relaxed);}}
    UiTimer(const UiTimer&)=delete;UiTimer& operator=(const UiTimer&)=delete;
};
template<class T>T Load(uintptr_t p){T value{};
    if(!p || !ds3content::Read(p,&value,sizeof(value)))
        throw std::runtime_error("native UI session field unreadable");return value;
}
template<class T>void Put(uintptr_t p,T value){std::memcpy(reinterpret_cast<void*>(p),&value,sizeof(value));}
// D-0182: the movie's name (movie+0x28 points to its UTF-16 name, as ObserveUiMovieAdvance reads it) is exactly `name`; an
// unreadable name is not that movie.
bool IsMovieNamed(uintptr_t movie,std::u16string_view name) noexcept {
    uintptr_t text{};std::array<char16_t,32> chars{};
    if(name.size()>=chars.size() || !movie || !ds3content::Read(movie+0x28,&text,sizeof(text)) || !text ||
       !ds3content::Read(text,chars.data(),(name.size()+1)*sizeof(char16_t)))return false;
    return std::u16string_view(chars.data(),name.size())==name && !chars[name.size()];
}
std::mutex registry_mutex;
std::unordered_map<uintptr_t,GameUiSession::State*> sessions;
// I-0067 SCALEFORM-FILTERS-NATIVE: CPU stand-ins for the original HAL's temporary render targets, shaped as the
// original filter code reads them (1F778D0, 1F7B930): target +0x28/+0x2C its buffer size, +0x30..+0x3C its rectangle
// in its texture, vt+0x18 its texture; texture +0x38/+0x3C its extent, vt+0x68 bind(stage, sampler byte*). They own no
// GPU object: `use` names the packet target of the target's current use (a new one per push, cleared at its first
// reference like the original's push clear), `sampled` the packet texture sampling that use.
struct FilterTexture {const uintptr_t* table{};std::array<uint8_t,0x30> unused{};uint32_t width{},height{};uint64_t use{},sampled{};};
struct FilterTarget {const uintptr_t* table{};std::array<uint8_t,0x20> unused{};uint32_t buffer_width{},buffer_height{};
    std::array<int32_t,4> rect{};FilterTexture texture;};
static_assert(offsetof(FilterTexture,width)==0x38 && offsetof(FilterTexture,height)==0x3C &&
    offsetof(FilterTarget,buffer_width)==0x28 && offsetof(FilterTarget,rect)==0x30);
}
struct GameUiSession::State {
    struct CpuFormat {
        uint32_t stride{},padding{};uintptr_t elements{},old_gpu_layout{};
        std::vector<CpuPrimitiveVertexElement> records;
    };
    uintptr_t module{},manager{},heap{},allocate{},free{},wrapper{},hal{},loader{},matrix_factory{};
    // Include the derived HAL draw entries above300. Unknown callbacks must
    // refuse inside this table rather than dispatch through adjacent storage.
    std::array<uintptr_t,128> table{};
    std::array<uintptr_t,3> wrapper_table{};
    std::recursive_mutex mutex;
    Failure failure{};
    GameUiImageOwner images;
    std::unique_ptr<GameUiMeshOwner> meshes;
    bool cpu_constructed{},glyph_initialized{},closed{},frame_open{},display_open{};
    std::vector<std::unique_ptr<CpuFormat>> formats;
    std::unique_ptr<GameUiCpuShaderPolicy> shader_policy;
    const frame::PrimitiveProgramBank* programs{};
    frame::PrimitiveWorkSource frame_source;
    // origin: the image object of an immutable snapshot this payload was copied from (frame_images holds the snapshot
    // for the frame, so the same object is the same bytes), or null.
    struct FrameTexture {uint64_t source_identity;size_t packet_index;const frame::PrimitiveTexture* origin{};};
    std::vector<FrameTexture> frame_textures;
    std::vector<UiImageIngress::Lease> frame_images;
    std::optional<frame::PrimitiveWorkPacket> prepared;
    uint64_t draw_identity{};
    struct BlendScope {uintptr_t primitive;uint32_t mode,flags;uint8_t premultiplied,target_alpha;};
    std::vector<BlendScope> blends;
    struct MaskScope {uintptr_t primitive;bool submitting;std::vector<frame::PrimitiveDraw> shapes;};
    std::vector<MaskScope> masks;
    std::vector<uintptr_t> cache_scopes;
    bool mask_clear_pending{};
    bool display_skipped{};   // ROADMAP workstream 4 step 1 (I-0137 tranche 5): a display family the native does not draw, drawn as an invisible one
    // I-0067 SCALEFORM-FILTERS-NATIVE: the native render-target stack (CPU parts of 1BDCBD0/1BDBB70: viewport block,
    // HAL+0x58 target bits, a genuine copy of the matrix state; masks and stencil mode are per target), the open
    // filter scopes (1BDC4E0/1BDB800) and this frame's target stand-ins and filter quad.
    struct TargetScope {std::array<uint8_t,0x3C> viewport{};uint32_t flags{};uintptr_t matrix{};uint64_t target{};
        uint32_t width{},height{};std::vector<MaskScope> masks;uint32_t depth_mode{},depth_reference{};bool mask_clear_pending{};};
    std::vector<TargetScope> target_stack;
    uint64_t current_target{};uint32_t current_width{},current_height{};
    struct FilterScope {uintptr_t primitive{};FilterTarget* subtree{};uint32_t depth_mode{},depth_reference{};};
    std::vector<FilterScope> filter_scopes;
    std::vector<std::unique_ptr<FilterTarget>> filter_targets;
    uint64_t filter_quad{};
    struct FilterCounts {uint64_t native{},unfiltered{},target_refusals{},passes{},cached{};} filter_counts;
    std::vector<std::string> filter_reports;std::vector<std::array<uint32_t,4>> filter_plans_seen,cache_plans_seen;uint64_t filter_frames{};
    uint64_t display_rects_fitted{};   // I-0076: display rectangles fitted to a shrunk extent (BeginCpuDisplay)
    // D-0182 (Thomas, 2026-11-12 02:48: the Bandai Namco screen on black, as in Elden Ring): dark_logo_page is true while
    // PrepareFrame calls the display of the movie named 05_901_Logo_BNE (its name at movie+0x28, the one the advance hook
    // journals); dark_logo_display while a display begun inside that call is open (BeginCpuDisplay to EndCpuDisplay): only its
    // draws are the page's. Display calls beginDisplay directly (STATIC, I-0067 MENUS-MAP: FUN_141BDA3A0); a display begun
    // elsewhere (deferred to EndCpuFrame's 1BDAE90) is not marked, so the page would stay as the game draws it.
    // The journal (I-0135): dark_logo_calls the calls that matched, dark_logo_displays the displays begun inside this call,
    // dark_logo_draw its marked draws since it began; dark_logo_frames the calls with a marked draw, the first at
    // dark_logo_first_tick; the first two of them and the first 1.5 s or more after it are journaled (dark_logo_journal).
    bool dark_logo_page{},dark_logo_display{},dark_logo_journal{},dark_logo_late{};uint32_t dark_logo_draw{},dark_logo_displays{};
    uint64_t dark_logo_calls{},dark_logo_frames{},dark_logo_first_tick{};
    // The page on black (DarkLogoAdapt): the logo drawn on black (made from the kept bytes, or Elden Ring's) and the resource of
    // the game's logo it replaces, kept while the page shows (DarkLogoRelease);
    // dark_logo_dimmed and dark_logo_imaged count this call's untextured draws turned to Multiply and image draws turned;
    // dark_logo_refused, an image the rule refused (then the page stays as the game draws it).
    std::vector<uint8_t> dark_logo_image;Microsoft::WRL::ComPtr<ID3D12Resource> dark_logo_resource;
    uint32_t dark_logo_dimmed{},dark_logo_imaged{},dark_logo_restored{};bool dark_logo_refused{};
    // The final review (M1, L2): the rectangles dimmed in the open display (their index in frame_source.draws and their blend),
    // given back their blend at its end unless its image was replaced; dark_logo_absent, the frames since the page was last
    // displayed; dark_logo_done once it has been gone 60 frames (it shows once, at the start): everything released, the name no
    // longer looked for.
    std::vector<std::pair<size_t,frame::PrimitiveBlend>> dark_logo_dimmed_at;uint32_t dark_logo_imaged_display{},dark_logo_absent{};
    bool dark_logo_done{};
    bool dark_logo_er_tried{},dark_logo_er{};   // Elden Ring's logo beside the module (Thomas, 05:40): looked for once, found
    State(uintptr_t m,uintptr_t receiver,TextureIngress& textures,DeviceQueues& gpu,std::mutex& submissions,uint64_t incarnation,Failure fail)
        :module(m),manager(receiver),failure(fail),images(m,textures,gpu,submissions,incarnation,fail),meshes(std::make_unique<GameUiMeshOwner>(m,fail)){}
    [[noreturn]] void Fail(const char* why) const noexcept {failure(why);std::terminate();}
    // I-0101 ("skip the object and go on", FABLE-REVIEW 4.2): a Scaleform family never witnessed (a blend mode, a primitive
    // state, a mesh kind) skips that one draw and is counted, where no scope or target state is left half-advanced; a row
    // `native_ui_skipped` for its first four times and every 1,000th, through the frame's reports.
    std::map<std::string,uint64_t> skipped;
    void Skip(const char* why,uint64_t detail=0){const auto n=++skipped[why];
        if((n<=4 || n%1000==0) && filter_reports.size()<256){char row[192]{};
            sprintf_s(row,"{\"event\":\"native_ui_skipped\",\"reason\":\"%s\",\"detail\":%llu,\"times\":%llu}\n",why,static_cast<unsigned long long>(detail),static_cast<unsigned long long>(n));
            filter_reports.emplace_back(row);}}
    uintptr_t Allocate(size_t bytes){const uint32_t hint=0x48;
        const auto p=reinterpret_cast<uintptr_t(*)(uintptr_t,size_t,const uint32_t*)>(allocate)(heap,bytes,&hint);
        if(!p)throw std::bad_alloc();std::memset(reinterpret_cast<void*>(p),0,bytes);return p;
    }
    void Free(uintptr_t p)const {reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(free)(heap,p);}
    uintptr_t Format(uint32_t stride,std::vector<CpuPrimitiveVertexElement> records){
        records.push_back({0,0});
        for(const auto& known:formats)if(known->stride==stride && known->records.size()==records.size() &&
            std::equal(records.begin(),records.end(),known->records.begin(),[](const auto& a,const auto& b){return a.offset==b.offset && a.attributes==b.attributes;}))
            return reinterpret_cast<uintptr_t>(known.get());
        auto format=std::make_unique<CpuFormat>();format->stride=stride;format->records=std::move(records);
        format->elements=reinterpret_cast<uintptr_t>(format->records.data());
        const auto pointer=reinterpret_cast<uintptr_t>(format.get());formats.push_back(std::move(format));return pointer;
    }
};
namespace {
auto& Owner(uintptr_t hal){const std::lock_guard<std::mutex> lock(registry_mutex);
    const auto found=sessions.find(hal);if(found==sessions.end())std::terminate();return *found->second;
}
template<size_t Slot>[[noreturn]] void Unsupported(uintptr_t hal){auto& state=Owner(hal);
    char reason[96]{};sprintf_s(reason,"native_ui_session_method_%03zX",Slot*8);state.Fail(reason);
}
template<size_t... Slots>void DefaultMethods(GameUiSession::State& state,std::index_sequence<Slots...>){
    state.table={reinterpret_cast<uintptr_t>(&Unsupported<Slots>)...};
}
uintptr_t ImageManager(uintptr_t hal){auto& state=Owner(hal);const std::lock_guard<std::recursive_mutex> lock(state.mutex);
    if(state.closed)state.Fail("native_ui_session_closed_image_service");return reinterpret_cast<uintptr_t>(state.images.ConstructManager());
}
uint32_t ImageServicesAvailable(uintptr_t hal){const auto manager=ImageManager(hal);if(!manager)return 0;   // I-0120 G9: no R8 sampling, no services
    const auto table=Load<uintptr_t>(manager);
    const auto caps=reinterpret_cast<uint32_t(*)(uintptr_t,uint32_t)>(Load<uintptr_t>(table+0x20))(manager,9);
    return (caps&0x20) && !(caps&0x80)?1u:0u;
}
void NativeCacheScope(uintptr_t hal,uintptr_t primitive,uint8_t ending){auto& state=Owner(hal);
    if(!state.display_open || !primitive || ending>1)state.Fail("native_ui_cache_scope_admission");
    if(ending){
        // The end comes from the queue's end marker (1C64340 with FilterSet nil, 1C642F0 with mode 0x15): a primitive of
        // its own, not the one that opened the scope (Native1265). Scopes nest; the innermost closes.
        if(state.cache_scopes.empty())state.Fail("native_ui_cache_scope_unbalanced");
    }else {
        if(state.cache_scopes.size()>=1024)state.Fail("native_ui_cache_scope_extent");
        // The genuine CPU cache policy is reusable while no cached GPU target
        // exists. A foreign target needs a native cache resource join first.
        if(Load<uint32_t>(primitive+0x18) && (Load<uintptr_t>(primitive+0x20) || Load<uintptr_t>(primitive+0x28)))
            state.Fail("native_ui_cached_filter_target_not_owned");
        state.cache_scopes.push_back(primitive);
    }
    reinterpret_cast<void(*)(uintptr_t,uintptr_t,uint8_t)>(state.module+0x1BDBFC0)(hal,primitive,ending);
    if(ending)state.cache_scopes.pop_back();
}
void PushCpuUserData(uintptr_t hal,uintptr_t data){auto& state=Owner(hal);
    if(!state.display_open || Load<uint64_t>(hal+0x1C8)>=1024)state.Fail("native_ui_userdata_push_scope");
    reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(state.module+0x1BDD010)(hal,data);
}
void PopCpuUserData(uintptr_t hal){auto& state=Owner(hal);
    if(!state.display_open || !Load<uint64_t>(hal+0x1C8))state.Fail("native_ui_userdata_pop_scope");
    reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BDBEC0)(hal);
}
uint32_t BeginCpuFrame(uintptr_t hal){auto& state=Owner(hal);
    if(state.closed || state.frame_open)state.Fail("native_ui_cpu_frame_admission");
    state.frame_open=true;
    // Only the actual CPU frame-active bit. The old HAL initialized/GPU-ready
    // bit remains absent; unknown rendering methods do not become available.
    Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)|2u);
    reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1C68A20)(Load<uintptr_t>(hal+0x240));
    reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1C703E0)(Load<uintptr_t>(hal+0x248));
    const auto fills=reinterpret_cast<uintptr_t(*)(uintptr_t)>(state.table[0xC8/8])(hal);
    reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1C65A80)(fills);
    state.images.ConsumeCpuPending();
    return 1;
}
void EndCpuFrame(uintptr_t hal){auto& state=Owner(hal);
    if(state.frame_open){
        // Glyph preparation may defer queue emission until the scene finishes.
        // Complete the actual CPU queue (1BDAE90, wait mode2), including queued
        // EndDisplay, before testing scope closure or retiring CPU frame locks.
        reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BDAE90)(hal);
    }
    if(!state.frame_open || state.display_open){char reason[160]{};
        sprintf_s(reason,"native_ui_cpu_frame_end_scope frame%u display%u caller%llX flags%X",unsigned(state.frame_open),unsigned(state.display_open),
            static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(_ReturnAddress())-state.module),Load<uint32_t>(hal+0x58));state.Fail(reason);}
    state.images.ConsumeCpuPending();
    reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BD5FE0)(hal+0x10);
    Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)&~2u);state.frame_open=false;
}
void NativeViewport(uintptr_t hal){auto& state=Owner(hal);
    if(!state.display_open)state.Fail("native_ui_viewport_without_display");
    const auto matrix=Load<uintptr_t>(hal+0x258);
    if(Load<uint32_t>(hal+0x58)&0x20){
        const auto left=Load<int32_t>(hal+0x1170),top=Load<int32_t>(hal+0x1174);
        const auto right=Load<int32_t>(hal+0x1178),bottom=Load<int32_t>(hal+0x117C);
        if(right<=left || bottom<=top)state.Fail("native_ui_empty_projection");
        // Actual CPU projection and matrix update from1F7BFF0, excluding its
        // final call into the old device/context viewport setter.
        reinterpret_cast<void(*)(uintptr_t,uint32_t,uintptr_t,uintptr_t,int32_t,int32_t)>(state.module+0x1BDE4F0)
            (hal,0,matrix+0x10,hal+0x1170,left-Load<int32_t>(hal+0x114C),top-Load<int32_t>(hal+0x1150));
        reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(state.module+0x1C672D0)(matrix,matrix+0x120);
        std::memcpy(reinterpret_cast<void*>(matrix+0x1D0),reinterpret_cast<void*>(hal+0x1170),16);
        Put<uint8_t>(matrix+0x280,1);
    }
}
void BeginCpuDisplay(uintptr_t hal,uintptr_t record){auto& state=Owner(hal);
    if(state.closed || !state.frame_open || state.display_open || !record)
        state.Fail("native_ui_display_admission");
    // Initial direct-display family. Offscreen composition and an authored colour-clear command require their own native
    // submissions, never a stub. ROADMAP workstream 4 step 1 (I-0137 tranche 5): such a display no longer ends the game: it opens
    // and closes as an invisible one (HAL+0x58 bit 0x20 clear: no primitive or complex mesh prepared, filters unfiltered; its scopes,
    // masks and targets kept balanced), counted once a display; a draw reaching RecordUiDraw in it fails (the invariant broken).
    state.display_skipped=false;
    if(Load<uint8_t>(record+0x40)){state.Skip("display_offscreen_unavailable");state.display_skipped=true;}
    else if((Load<uint32_t>(record+0x10)>>24) && !(Load<uint32_t>(record+0x3C)&0x200)){state.Skip("display_colour_clear_unavailable",Load<uint32_t>(record+0x10)>>24);state.display_skipped=true;}
    state.display_open=true;state.dark_logo_display=state.dark_logo_page;if(state.dark_logo_page)++state.dark_logo_displays;   // D-0182
    state.dark_logo_dimmed_at.clear();state.dark_logo_imaged_display=0;
    // HAL1128 is the CPU record pool's free-list head (pool1110+18),
    // not an active display stack. The original Begin immediately recycles
    // this record; retain only the copied viewport/projection values below.
    Put<uintptr_t>(record,Load<uintptr_t>(hal+0x1128));Put<uintptr_t>(hal+0x1128,record);
    // Frame/scene/display-active flags describe the admitted CPU scope only.
    // The original GPU initialization flag1 is never set.
    Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)|4u|8u);
    Put<uint8_t>(hal+0x210,static_cast<uint8_t>((Load<uint32_t>(record+0x3C)>>1)&1));
    reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BDE4E0)(hal);
    std::memcpy(reinterpret_cast<void*>(hal+0x1180),reinterpret_cast<void*>(record+0x50),32);
    std::array<int32_t,12> viewport{};
    const auto matrix=Load<uintptr_t>(hal+0x258);
    reinterpret_cast<uintptr_t(*)(uintptr_t,int32_t*,uintptr_t)>(state.module+0x1C66C10)(matrix,viewport.data(),record+0x14);
    std::memcpy(reinterpret_cast<void*>(hal+0x1144),viewport.data(),44);
    const auto visible=reinterpret_cast<uint8_t(*)(uintptr_t,uintptr_t,uint8_t)>(state.module+0x1BD8050)(hal+0x1144,hal+0x1170,0);
    Put<uint32_t>(hal+0x58,(Load<uint32_t>(hal+0x58)&~0x20u)|(visible && !state.display_skipped?0x20u:0u));   // a skipped display draws as an invisible one
    // I-0076: the frame after the output shrinks (a screen mode or resolution applied), a movie can still carry the former
    // extent's display rectangle; the display draws' viewport and scissor (this rectangle) are fitted to the frame's extent,
    // as the original's D3D11 rasterizer clipped them to the back buffer, instead of refusing the frame.
    {const int32_t width=static_cast<int32_t>(state.frame_source.width),height=static_cast<int32_t>(state.frame_source.height);
        const std::array<int32_t,4> rect{Load<int32_t>(hal+0x1170),Load<int32_t>(hal+0x1174),Load<int32_t>(hal+0x1178),Load<int32_t>(hal+0x117C)};
        if(rect[0]<0 || rect[1]<0 || rect[2]>width || rect[3]>height){
            const std::array<int32_t,4> fitted{std::clamp(rect[0],0,width),std::clamp(rect[1],0,height),std::clamp(rect[2],0,width),std::clamp(rect[3],0,height)};
            for(size_t i=0;i<4;++i)Put<int32_t>(hal+0x1170+4*i,fitted[i]);
            if(fitted[0]>=fitted[2] || fitted[1]>=fitted[3])Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)&~0x20u);   // review L4: nothing left to draw
            if(++state.display_rects_fitted<=32 && state.filter_reports.size()<256){char row[192]{};
                sprintf_s(row,"{\"event\":\"native_ui_display_rect_fitted\",\"serial\":%llu,\"rect\":[%d,%d,%d,%d],\"extent\":[%d,%d]}\n",
                    static_cast<unsigned long long>(state.frame_source.serial),rect[0],rect[1],rect[2],rect[3],width,height);state.filter_reports.emplace_back(row);}}}
    NativeViewport(hal);
}
void EndCpuDisplay(uintptr_t hal){auto& state=Owner(hal);
    if(!state.display_open || !state.blends.empty() || !state.masks.empty() || Load<uint64_t>(hal+0x180) ||
       !state.cache_scopes.empty() || Load<uint64_t>(hal+0x1B0) || Load<uint64_t>(hal+0x1C8) ||
       !state.filter_scopes.empty() || !state.target_stack.empty() || state.current_target){char reason[200]{};
        sprintf_s(reason,"native_ui_display_end_scope open%u blends%zu masks%zu filters%llu/%zu caches%zu targets%llu/%zu/%llu user%llu",
            unsigned(state.display_open),state.blends.size(),state.masks.size(),static_cast<unsigned long long>(Load<uint64_t>(hal+0x180)),
            state.filter_scopes.size(),state.cache_scopes.size(),static_cast<unsigned long long>(Load<uint64_t>(hal+0x1B0)),state.target_stack.size(),
            static_cast<unsigned long long>(state.current_target),static_cast<unsigned long long>(Load<uint64_t>(hal+0x1C8)));state.Fail(reason);}
    // Begin already returned the record to its real CPU pool. It may have
    // been reused before deferred EndDisplay emits; never read it or pop the
    // pool head here. Frame completion separately drains the actual CPU queue.
    Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)&~(4u|8u|0x20u));
    // D-0182 (final review M1): a display of the page whose image was not replaced gives its rectangles back their blend, so the
    // page is never black without its logo (a changed or unexpected image is drawn as the game draws it).
    if(state.dark_logo_display && !state.dark_logo_imaged_display)
        for(const auto& [at,blend]:state.dark_logo_dimmed_at)if(at<state.frame_source.draws.size()){
            state.frame_source.draws[at].blend=blend;--state.dark_logo_dimmed;++state.dark_logo_restored;}   // step review L2: journaled
    state.dark_logo_dimmed_at.clear();
    state.display_open=false;state.dark_logo_display=false;   // D-0182
    state.mask_clear_pending=false;state.display_skipped=false;
}
void PushNativeMask(uintptr_t hal,uintptr_t primitive){auto& state=Owner(hal);
    if(!state.display_open || !primitive || state.masks.size()>=255 || (!state.masks.empty() && state.masks.back().submitting))
        state.Fail("native_ui_mask_push_scope");
    // Rebuild current mask intersections from owned immutable geometry.
    // Native stencil storage replaces old targets and stale-region GPU caches.
    state.mask_clear_pending=true;
    for(size_t level=0;level<state.masks.size();++level)for(const auto& shape:state.masks[level].shapes){
        auto replay=shape;replay.identity=++state.draw_identity;
        replay.stencil=frame::PrimitiveStencil::WriteMaskLevel;replay.stencil_reference=static_cast<uint32_t>(level);
        replay.clear_stencil_before=std::exchange(state.mask_clear_pending,false);
        state.frame_source.draws.push_back(std::move(replay));
    }
    reinterpret_cast<uint32_t(*)(uintptr_t)>(state.module+0x1BB6C10)(primitive);
    try{state.masks.push_back({primitive,true,{}});}catch(...){reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BB6CA0)(primitive);throw;}
    Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)|0x40u);
    Put<uint32_t>(hal+0x1F8,4);Put<uint32_t>(hal+0x1FC,static_cast<uint32_t>(state.masks.size()-1));
}
void EndNativeMask(uintptr_t hal){auto& state=Owner(hal);
    if(!state.display_open || state.masks.empty() || !state.masks.back().submitting)state.Fail("native_ui_mask_end_scope");
    state.masks.back().submitting=false;
    Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)&~0x40u);
    Put<uint32_t>(hal+0x1F8,5);Put<uint32_t>(hal+0x1FC,static_cast<uint32_t>(state.masks.size()));
}
void PopNativeMask(uintptr_t hal){auto& state=Owner(hal);
    if(!state.display_open || state.masks.empty() || state.masks.back().submitting)state.Fail("native_ui_mask_pop_scope");
    const auto primitive=state.masks.back().primitive;state.masks.pop_back();
    reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BB6CA0)(primitive);
    Put<uint32_t>(hal+0x1F8,state.masks.empty()?1u:5u);Put<uint32_t>(hal+0x1FC,static_cast<uint32_t>(state.masks.size()));
}
void MapCpuVertexFormat(uintptr_t hal,uint32_t fill,uintptr_t source,uintptr_t normal,uintptr_t batched,uintptr_t instanced){
    auto& state=Owner(hal);
    if(!state.frame_open || !source || !normal || !batched || !instanced || fill>15)
        state.Fail("native_ui_vertex_layout_admission");
    if(Load<uint32_t>(hal+0x39F18)>1)state.Fail("native_ui_vertex_layout_instancing_unavailable");
    std::vector<CpuPrimitiveVertexElement> normalized;const auto elements=Load<uintptr_t>(source+8);
    bool terminated{};
    //1F75BF0's real CPU position normalization; no input-layout wrapper.
    for(size_t n=0;n<16;++n){auto element=Load<CpuPrimitiveVertexElement>(elements+n*8);
        if(!element.attributes){terminated=true;break;}
        if((element.attributes&0xF00)==0x100)element.attributes=(element.attributes&0xFFFFFF6F)|0x60;
        normalized.push_back(element);
    }
    if(!terminated || normalized.empty())state.Fail("native_ui_vertex_layout_source_extent");
    uint32_t selector=0x80000;
    switch(fill){case 3:selector=0x401;break;case 4:selector=0x421;break;case 5:selector=1;break;
        case 6:selector=0x21;break;case 7:selector=0x2001;break;case 8:selector=0x2021;break;
        case 9:selector=0x801;break;case 10:selector=0x821;break;case 11:selector=0x90000;break;case 12:selector=0x88000;break;}
    // Pure static installed CPU programme descriptor lookup, using this
    // renderer's already established mode3 programme policy.
    const auto descriptor=reinterpret_cast<uintptr_t(*)(uint32_t,uint32_t)>(state.module+0x1F855C0)(selector,3);
    if(!descriptor)state.Fail("native_ui_vertex_layout_program_missing");
    const auto count=Load<uint8_t>(descriptor+0x2C);
    if(!count || count>8)state.Fail("native_ui_vertex_layout_program_extent");
    std::vector<CpuPrimitiveVertexElement> output;
    uint32_t stride{},alignment=4,factor_offset=~0u;size_t factor_index{};
    constexpr uint32_t widths[]{1,1,2,2,4,4};
    for(uint32_t n=0;n<count;++n){const auto semantic=Load<uint32_t>(descriptor+0x38+size_t(n)*32);
        if((semantic&0xFF0F)==0x1204){
            factor_index=output.size()+1;factor_offset=stride+2;
            output.push_back({stride,0x2211});output.push_back({stride+3,0x1211});stride+=4;continue;
        }
        const auto found=std::find_if(normalized.begin(),normalized.end(),[&](const auto& e){return (e.attributes&0xFF00)==(semantic&0xFF00);});
        if(found==normalized.end())state.Fail("native_ui_vertex_layout_missing_semantic");
        auto attributes=found->attributes;
        if((attributes&0xFFF)==0x251)attributes=(attributes&0xFFFFF214)|0x214;
        const auto type=(attributes>>4)&15,components=attributes&15;
        if(!type || type>6 || !components || components>4)state.Fail("native_ui_vertex_layout_element_type");
        alignment=std::max(alignment,widths[type-1]);output.push_back({stride,attributes});
        stride=(stride+widths[type-1]*components+3)&~3u;
    }
    const auto single_stride=(stride+alignment-1)&~(alignment-1);
    Put<uintptr_t>(normal,state.Format(single_stride,output));
    // Genuine CPU batch index metadata. Whether a later writer can generate
    // that family remains its own explicit admission, with old GPU slot nil.
    if(factor_offset==~0u){output.push_back({stride,0x10421});stride=(stride+4)&~3u;}
    else {output.insert(output.begin()+factor_index,{factor_offset,0x10421});}
    Put<uintptr_t>(batched,state.Format((stride+alignment-1)&~(alignment-1),std::move(output)));
    Put<uintptr_t>(instanced,0);
}
uint32_t NativeBatchCapacity(uintptr_t hal,uintptr_t){auto& state=Owner(hal);
    if(state.closed)state.Fail("native_ui_batch_capacity_after_close");
    // The owned writer/shader transaction currently supports one mesh. This
    // native capacity makes real CPU batch preparation split at that boundary,
    // rather than borrowing the old shader manager's uniform/GPU limits.
    return 1;
}
void NativeBlendState(uintptr_t hal,uint32_t mode,uint8_t premultiplied,uint8_t target_alpha){auto& state=Owner(hal);
    if(!state.display_open)state.Fail("native_ui_blend_state_scope");
    // I-0101: a mode outside the witnessed set is kept as set; a draw under it that DecodeUiPrimitiveState cannot decode is skipped.
    if((mode!=0 && mode!=1 && mode!=3 && mode!=4 && mode!=8 && mode!=18) || premultiplied>1 || target_alpha>1)state.Skip("blend_state_family",mode);
    Put<uint32_t>(hal+0x20C,mode);Put<uint8_t>(hal+0x210,premultiplied);Put<uint8_t>(hal+0x211,target_alpha);
}
void PushNativeBlend(uintptr_t hal,uintptr_t primitive){auto& state=Owner(hal);
    if(!state.display_open || !primitive || state.blends.size()>=64)state.Fail("native_ui_blend_push_scope");
    // I-0101 final review M2: a mode outside the witnessed set goes on to 1BD9D90 (the game's own logic: it returns when the
    // state already matches, draws a mode above 0x13 as 0, and maps the mode before 0x288), so NativeBlendState sees the mapped
    // mode, counts one outside the witnessed set and keeps it, and the colour draws under it are skipped there.
    const auto mode=Load<uint32_t>(primitive+0x48);
    GameUiSession::State::BlendScope scope{primitive,Load<uint32_t>(hal+0x20C),Load<uint32_t>(hal+0x214),
        Load<uint8_t>(hal+0x210),Load<uint8_t>(hal+0x211)};
    reinterpret_cast<uint32_t(*)(uintptr_t)>(state.module+0x1BB6C10)(primitive);
    try{state.blends.push_back(scope);}catch(...){reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BB6CA0)(primitive);throw;}
    // Genuine CPU profile/flag selection, with native288 storing semantic
    // blend policy. No original blend-state object or offscreen target exists.
    reinterpret_cast<void(*)(uintptr_t,uint32_t,uint8_t,uint8_t)>(state.module+0x1BD9D90)
        (hal,mode,0,static_cast<uint8_t>((Load<uint32_t>(hal+0x58)>>4)&1));
}
void PopNativeBlend(uintptr_t hal){auto& state=Owner(hal);
    if(!state.display_open || state.blends.empty())state.Fail("native_ui_blend_pop_scope");
    const auto scope=state.blends.back();state.blends.pop_back();
    Put<uint32_t>(hal+0x20C,scope.mode);Put<uint32_t>(hal+0x214,scope.flags);
    Put<uint8_t>(hal+0x210,scope.premultiplied);Put<uint8_t>(hal+0x211,scope.target_alpha);
    reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BB6CA0)(scope.primitive);
}
// The viewport of a draw recorded now: the pushed target's rectangle while a target pushed with flag 8 is current
// (1F7BFF0, HAL+0x58 bit 0x10), else the display rectangle.
frame::PrimitiveViewport CurrentUiViewport(uintptr_t hal){
    if(Load<uint32_t>(hal+0x58)&0x10)return {static_cast<float>(Load<int32_t>(hal+0x114C)),static_cast<float>(Load<int32_t>(hal+0x1150)),
        static_cast<float>(Load<int32_t>(hal+0x1154)),static_cast<float>(Load<int32_t>(hal+0x1158)),0,0};
    const auto left=Load<int32_t>(hal+0x1170),top=Load<int32_t>(hal+0x1174);
    return {static_cast<float>(left),static_cast<float>(top),static_cast<float>(Load<int32_t>(hal+0x1178)-left),static_cast<float>(Load<int32_t>(hal+0x117C)-top),0,0};
}
void AdmitFrameProgram(GameUiSession::State& state,frame::PrimitiveProgram program){
    const auto found=std::find_if(state.frame_source.programs.begin(),state.frame_source.programs.end(),[&](const auto& p){return p.identity==program.identity;});
    if(found==state.frame_source.programs.end())state.frame_source.programs.push_back(std::move(program));
    else if(found->program.bytecode!=program.program.bytecode)state.Fail("native_ui_frame_program_conflict");
}
// Scissor, pending mask clear, mask shape capture and the current target of a draw recorded now.
void RecordUiDraw(GameUiSession::State& state,uintptr_t hal,frame::PrimitiveDraw draw){
    // Step 1: a skipped display is an invisible one (BeginCpuDisplay), whose producers never reach here; one that does breaks that
    // invariant after its uploads were made (the final review's L1), so it fails.
    if(state.display_skipped)state.Fail("native_ui_skipped_display_draw");
    if(Load<uint32_t>(hal+0x116C)&4){draw.scissor_enabled=true;
        draw.scissor={Load<int32_t>(hal+0x1170),Load<int32_t>(hal+0x1174),Load<int32_t>(hal+0x1178),Load<int32_t>(hal+0x117C)};}
    draw.clear_stencil_before=std::exchange(state.mask_clear_pending,false);
    draw.target=state.current_target;
    if(!state.masks.empty() && state.masks.back().submitting)state.masks.back().shapes.push_back(draw);
    state.frame_source.draws.push_back(std::move(draw));
}
// D-0182 (I-0135): counts the page's frames at a display call's first marked draw (the page rows below journal the first two
// and the first 1.5 s or more after the first). The per-draw rows that read the page's layers were removed when I-0135 closed
// (final review L4); the run start-732bb5ed5 keeps them.
void DarkLogoCount(GameUiSession::State& state){
    if(state.dark_logo_draw++)return;
    const auto now=GetTickCount64();
    if(!state.dark_logo_frames++)state.dark_logo_first_tick=now;
    const bool late=!state.dark_logo_late && now-state.dark_logo_first_tick>=1500;state.dark_logo_late|=late;
    state.dark_logo_journal=state.dark_logo_frames<=2 || late;
}
// D-0182 (I-0135): after each display call of the page, one row for a journaled frame and for its first three calls without a
// marked draw (an empty journal then says which: the name never matched, no display begun inside the call, or no draw).
void DarkLogoPageReport(GameUiSession::State& state){
    if(!(state.dark_logo_draw?state.dark_logo_journal:state.dark_logo_calls<=3) || state.filter_reports.size()>=256)return;
    char row[320]{};sprintf_s(row,"{\"event\":\"native_ui_dark_logo_page\",\"call\":%llu,\"page_frame\":%llu,\"ms\":%llu,\"displays\":%u,\"draws\":%u,"
        "\"dimmed\":%u,\"imaged\":%u,\"restored\":%u,\"refused\":%s,\"er\":%s}\n",
        static_cast<unsigned long long>(state.dark_logo_calls),static_cast<unsigned long long>(state.dark_logo_draw?state.dark_logo_frames:0),
        static_cast<unsigned long long>(state.dark_logo_frames?GetTickCount64()-state.dark_logo_first_tick:0),state.dark_logo_displays,state.dark_logo_draw,
        state.dark_logo_dimmed,state.dark_logo_imaged,state.dark_logo_restored,state.dark_logo_refused?"true":"false",state.dark_logo_er?"true":"false");
    state.filter_reports.emplace_back(row);
}
// D-0182 (I-0135): the page on black. Its layers (05_901_logo_bne.gfx, read statically): a black full-screen rectangle, then in
// the sprite Top (whose alpha makes the fades) a white full-screen rectangle and the image MENU_BNE_Logo, 768x768 BC1, opaque
// white around its blob, drawn in Multiply over that white. Here, on the page's draws only, before their images take the
// frame's identities:
// - an untextured draw is drawn in Multiply: the black rectangle stays black and the white one leaves the black beneath it
//   (multiplying by white changes nothing), whatever encodes their colour (a fill constant or vertex colours);
// - the image's Multiply draw samples the logo made for black (frame::DarkLogoImage, straight alpha, made once from the game's
//   bytes and kept) and blends in premultiplied alpha: the program of a Multiply draw outputs its colour premultiplied by its
//   alpha (INFERRED: the Multiply row's DEST_COLOR / INV_SRC_ALPHA, primitive_pass.cpp, assumes it), so Top's fades stay
//   fades to black. The page's images are not lent (each draw replaces its own copy). An image of another shape, or one the rule
//   refuses, is left as the game draws it (dark_logo_refused, in the page rows).
// - Thomas, 2026-11-12 05:40 (I-0135 CONTRACT, his PC only): when er-bne-logo.dds lies beside the module (written once by the
//   local script Extract-EldenRingLogo.ps1 from his own Elden Ring: its 2022 logo fitted to this page's square, opaque on
//   black), the image's draw samples it instead, the same way; read once, at the page's first draw (`er` in the page rows).
bool LoadErLogo(std::vector<uint8_t>& rgba){
    // The file beside this module (game_bootstrap.cpp's ModuleSibling lives in that unit's anonymous namespace).
    std::array<wchar_t,MAX_PATH> module{};HMODULE self{};
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&LoadErLogo),&self))return false;
    if(const auto chars=GetModuleFileNameW(self,module.data(),MAX_PATH);!chars || chars>=MAX_PATH)return false;
    std::wstring path(module.data());path=path.substr(0,path.find_last_of(L"\\/")+1)+L"er-bne-logo.dds";
    std::ifstream in(path,std::ios::binary);if(!in)return false;
    const std::vector<uint8_t> file((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
    return frame::ParseLogoDds(file,768,rgba);
}
// - The run start-732bb5ed5 (I-0135 RESULT): the game's image is a resident resource bound as is (`external`), its bytes freed
//   at its upload (I-0096); the admission keeps a copy of that one texture by its TPF entry name (dark_logo_kept.h), from
//   which the logo for black is made once (the copy then freed), and the image is recognised by its resource. The rectangles
//   are dimmed only when the image will be replaced (Elden Ring's logo, or the logo for black made): never a black page
//   without its logo; with neither, the page stays as the game draws it.
// Returns the source identity of the image it replaced (0 if none): the frame registers it without its snapshot origin, so no
// other draw of the frame is lent the replaced copy (final review L1; the page's own draws are not lent either).
uint64_t DarkLogoAdapt(GameUiSession::State& state,frame::PrimitiveWorkPacket& packet){
    if(!state.dark_logo_er_tried){state.dark_logo_er_tried=true;state.dark_logo_er=LoadErLogo(state.dark_logo_image);}
    // Without Elden Ring's logo: the logo for black, made once from the game's bytes the admission kept (dark_logo_kept.h,
    // BC1 768x768 only), which are then freed; the page's image is that upload's resource.
    if(!state.dark_logo_er && !state.dark_logo_resource && !state.dark_logo_refused){auto& kept=frame::DarkLogoKept();
        const std::lock_guard hold(kept.lock);
        if(kept.resource && !kept.bc1.empty()){std::vector<uint8_t> made;
            if(kept.width==768 && kept.height==768 && frame::DarkLogoImage(kept.width,kept.height,kept.bc1,made)){
                state.dark_logo_image=std::move(made);state.dark_logo_resource=kept.resource;}
            else state.dark_logo_refused=true;
            kept.bc1=std::vector<uint8_t>{};}}
    const bool ready=state.dark_logo_er || (state.dark_logo_resource && !state.dark_logo_image.empty());
    auto& draw=packet.draws.front();
    if(!ready || draw.blend==frame::PrimitiveBlend::NoColor)return 0;
    if(!draw.texture){state.dark_logo_dimmed_at.emplace_back(state.frame_source.draws.size(),draw.blend);   // where RecordUiDraw puts it
        draw.blend=frame::PrimitiveBlend::Multiply;++state.dark_logo_dimmed;return 0;}
    if(draw.blend!=frame::PrimitiveBlend::Multiply || draw.secondary.texture)return 0;
    for(auto& entry:packet.textures){if(entry.identity!=draw.texture || entry.target)continue;
        auto& image=entry.texture;
        // The game's logo: Elden Ring's file replaces it whichever it is; otherwise only the resource the kept bytes came from.
        if(image.format!=DXGI_FORMAT_BC1_UNORM || image.width!=768 || image.height!=768 ||
           !(state.dark_logo_er || (entry.external && entry.external==state.dark_logo_resource)))continue;
        entry.external.Reset();image.format=DXGI_FORMAT_R8G8B8A8_UNORM;image.mips.assign(1,state.dark_logo_image);
        draw.blend=frame::PrimitiveBlend::PremultipliedAlpha;++state.dark_logo_imaged;++state.dark_logo_imaged_display;
        return entry.identity;
    }
    return 0;
}
// D-0182 (final review L2): once the page has gone (60 frames without it), the logo, its resource and the kept copy are released
// and the admission keeps nothing more.
void DarkLogoRelease(GameUiSession::State& state){
    state.dark_logo_done=true;state.dark_logo_image=std::vector<uint8_t>{};state.dark_logo_resource.Reset();
    auto& kept=frame::DarkLogoKept();const std::lock_guard hold(kept.lock);
    kept.closed=true;kept.resource.Reset();kept.bc1=std::vector<uint8_t>{};
}
void SubmitUiGeometry(uintptr_t hal,uintptr_t fill,uintptr_t format,uintptr_t nodes,const PrimitiveGeometry& geometry,
    uintptr_t texture_matrices=0,uint32_t texture_matrix_count=0){auto& state=Owner(hal);
        const UiTimer timer{2};if(timer.on)ui_profile[4].fetch_add(1,std::memory_order_relaxed);
        // I-0101: a primitive state outside the decoded families skips this draw before anything is prepared for it.
        UiPrimitiveState policy;
        if(!DecodeUiPrimitiveState(Load<uint32_t>(hal+0x1F8),Load<uint32_t>(hal+0x1FC),Load<uint32_t>(hal+0x20C),
            Load<uint8_t>(hal+0x210),Load<uint8_t>(hal+0x211),CurrentUiViewport(hal),policy)){state.Skip("primitive_state_family",Load<uint32_t>(hal+0x20C));return;}
        const auto id=++state.draw_identity,serial=state.frame_source.serial;
        UiCpuPrepareInputs inputs{state.shader_policy->Manager(),fill,
            format,Load<uintptr_t>(hal+0x258),nodes,Load<uint32_t>(hal+0x214),0,1,texture_matrices,texture_matrix_count};
        UiCpuPreparedShader shader;UiDrawImageBinding image;const auto shader_start=UiStamp(timer.on);
        auto shader_status=frame::PrimitiveConstantStatus::Invalid;   // a refused binding scope never prepares: the module's contract
        const char* data_case{};   // tranche 5: set only for a case of the game's data (the selection's answer)
        const auto bound=state.images.PrepareBindings(serial,id,[&]{
            shader_status=PrepareNativeUiCpu(reinterpret_cast<UiCpuPrepare>(state.module+0x1F784C0),*state.programs,
                state.programs->Version(),3,serial,id,inputs,shader,state.failure,&data_case);
        },image);UiAdd(6,shader_start,timer.on);
        // ROADMAP workstream 4 step 1 (I-0137 tranche 5): a draw whose programme pair's uniform layout the native does not take is
        // skipped and counted (native_ui_skipped « layout_unsupported »), nothing of it recorded yet; every other refusal fails.
        if(shader_status!=frame::PrimitiveConstantStatus::Ready && data_case){state.Skip(data_case,static_cast<uint64_t>(shader_status));return;}
        if(shader_status!=frame::PrimitiveConstantStatus::Ready){char reason[96]{};   // its step review's M2: the module's contract
            sprintf_s(reason,"native_ui_primitive_shader_status_%u",static_cast<unsigned>(shader_status));state.Fail(reason);}
        // F4: a draw recorded inside a filtered subtree lands on the subtree's target, bounded by its extent.
        const auto width=state.current_target?state.current_width:state.frame_source.width;
        const auto height=state.current_target?state.current_height:state.frame_source.height;
        // An image object of an immutable snapshot the frame already copied is not copied again: the frame lends its
        // payload to this draw's packet (moved in, admitted, moved back below); its snapshot is held for the frame.
        if(bound && image.images && std::find(state.frame_images.begin(),state.frame_images.end(),image.images)==state.frame_images.end()){
            // Held: only the snapshots a recorded origin still points into (each publish copies the whole image set).
            std::erase_if(state.frame_images,[&](const auto& held){if(!held->textures)return true;const auto& all=held->textures->textures;
                const auto first=reinterpret_cast<uintptr_t>(all.data()),end=reinterpret_cast<uintptr_t>(all.data()+all.size());
                return std::none_of(state.frame_textures.begin(),state.frame_textures.end(),[&](const auto& e){
                    const auto at=reinterpret_cast<uintptr_t>(e.origin);return at>=first && at<end;});});
            state.frame_images.push_back(image.images);}
        struct Loan {uint64_t source_identity,native_identity;size_t slot;};std::vector<Loan> lent;lent.reserve(2);
        static const UiResidentImage not_lent{};   // D-0182 (final review L1)
        const UiResidentImage resident=[&](const frame::PrimitiveTexture& origin,frame::PrimitiveTexture& taken){
            const auto entry=std::find_if(state.frame_textures.begin(),state.frame_textures.end(),[&](const auto& e){return e.origin==&origin;});
            if(entry==state.frame_textures.end())return false;auto& slot=state.frame_source.textures[entry->packet_index];
            lent.push_back({origin.identity,slot.identity,entry->packet_index});taken=std::move(slot);return true;};
        const auto origin_of=[&](uint64_t identity)->const frame::PrimitiveTexture*{
            if(!bound || !image.images || !image.images->textures)return nullptr;
            for(const auto& t:image.images->textures->textures)if(t.identity==identity)return &t;
            return nullptr;};
        frame::PrimitiveWorkPacket packet;const auto primitive_start=UiStamp(timer.on);
        const auto result=bound?ProduceNativeUiPrimitive(shader,geometry,state.programs->Version(),serial,
            width,height,policy,image,packet,state.dark_logo_display?not_lent:resident):   // D-0182: the page's images not lent
            ProduceNativeUiPrimitive(shader,geometry,state.programs->Version(),serial,
            width,height,policy,packet);
        UiAdd(7,primitive_start,timer.on);
        // I-0137 tranche 5's step review H1: not a skip: the frame's payloads lent to this packet are lost when the producer refuses it.
        if(result.status!=frame::PrimitiveWorkStatus::Ready){char reason[96]{};
            sprintf_s(reason,"native_ui_primitive_packet_status_%u",static_cast<unsigned>(result.status));state.Fail(reason);}
        for(auto& program:packet.programs)AdmitFrameProgram(state,std::move(program));
        if(packet.draws.size()!=1)state.Fail("native_ui_primitive_draw_arity");
        const auto dark_logo_replaced=state.dark_logo_display?DarkLogoAdapt(state,packet):0;   // D-0182 (I-0135): the Bandai Namco page on black
        const auto vertex_identity=packet.draws.front().vertices,index_identity=packet.draws.front().indices;
        // Each prepared geometry slice owns its bytes. Its producer identity
        // need not belong to the cache allocator's namespace; remap references
        // once from the incoming ids into this ordered frame's buffer table.
        for(auto& buffer:packet.buffers){const auto incoming=buffer.identity;
            buffer.identity=state.frame_source.buffers.size()+1;
            if(incoming==vertex_identity)packet.draws.front().vertices=buffer.identity;
            if(incoming==index_identity)packet.draws.front().indices=buffer.identity;
            state.frame_source.buffers.push_back(std::move(buffer));}
        // This producer supplies one draw and up to its two images. A glyph
        // image can change between draws of the same CPU display transaction;
        // keep each observed payload under a frame-local native identity.
        if(packet.draws.size()!=1 || packet.textures.size()>2)state.Fail("native_ui_primitive_image_arity");
        const auto primary_texture=packet.draws.front().texture,secondary_texture=packet.draws.front().secondary.texture;
        for(auto& texture:packet.textures){const auto source_identity=texture.identity;
            uint64_t image_bytes{};if(timer.on){for(const auto& mip:texture.texture.mips)image_bytes+=mip.size();ui_profile[8].fetch_add(image_bytes,std::memory_order_relaxed);}
            uint64_t native_identity{};
            if(const auto loan=std::find_if(lent.begin(),lent.end(),[&](const auto& l){return l.source_identity==source_identity;});loan!=lent.end()){
                native_identity=loan->native_identity;texture.identity=native_identity;state.frame_source.textures[loan->slot]=std::move(texture);
                if(timer.on)ui_profile[9].fetch_add(image_bytes,std::memory_order_relaxed);
            } else {
            const auto found=std::find_if(state.frame_textures.begin(),state.frame_textures.end(),[&](const auto& entry){
                if(entry.source_identity!=source_identity)return false;
                const auto& known=state.frame_source.textures[entry.packet_index].texture;
                return state.frame_source.textures[entry.packet_index].external==texture.external && known.shader_slot==texture.texture.shader_slot && known.width==texture.texture.width &&
                    state.frame_source.textures[entry.packet_index].target==texture.target && known.height==texture.texture.height && known.format==texture.texture.format && known.mips==texture.texture.mips;
            });
            if(found!=state.frame_textures.end()){native_identity=state.frame_source.textures[found->packet_index].identity;
                found->origin=source_identity==dark_logo_replaced?nullptr:origin_of(source_identity);if(timer.on)ui_profile[9].fetch_add(image_bytes,std::memory_order_relaxed);}
            else {const auto index=state.frame_source.textures.size();native_identity=index+1;
                texture.identity=native_identity;state.frame_source.textures.push_back(std::move(texture));
                state.frame_textures.push_back({source_identity,index,source_identity==dark_logo_replaced?nullptr:origin_of(source_identity)});}
            }
            if(primary_texture!=source_identity && secondary_texture!=source_identity)state.Fail("native_ui_primitive_image_reference");
            if(primary_texture==source_identity)packet.draws.front().texture=native_identity;
            if(secondary_texture==source_identity)packet.draws.front().secondary.texture=native_identity;
        }
        if(state.dark_logo_display)DarkLogoCount(state);   // D-0182 (I-0135)
        for(auto& draw:packet.draws)RecordUiDraw(state,hal,std::move(draw));
        state.frame_source.texture_generation=std::max(state.frame_source.texture_generation,packet.texture_generation);
}
void EmitNativePrimitive(uintptr_t hal,uintptr_t primitive,uintptr_t first,uintptr_t end){auto& state=Owner(hal);
    if(!state.frame_open || !state.display_open || !state.programs || !state.shader_policy || !primitive)
        state.Fail("native_ui_primitive_scope");
    if(!(Load<uint32_t>(hal+0x58)&0x20) || !Load<uint32_t>(primitive+0x40))return;
    if(!first)first=Load<uintptr_t>(primitive+0x28);
    uint32_t visited{};
    for(auto batch=first;batch!=end;batch=Load<uintptr_t>(batch+8)){
        if(!batch || ++visited>4096)state.Fail("native_ui_primitive_batch_extent");
        const auto item=Load<uintptr_t>(batch+0x20);
        if(!item)continue; // Genuine CPU preparation has not supplied geometry.
        if(Load<uint32_t>(batch+0x48)!=1 || Load<uint32_t>(batch+0x28)){state.Skip("primitive_mesh_family",Load<uint32_t>(batch+0x48));continue;}   // I-0101
        const auto geometry=state.meshes->Acquire(item);
        if(!geometry)state.Fail("native_ui_primitive_geometry_missing");
        const auto nodes=Load<uintptr_t>(primitive+0x38)+uint64_t(Load<uint32_t>(batch+0x4C))*16;
        SubmitUiGeometry(hal,Load<uintptr_t>(primitive+0x18),Load<uintptr_t>(batch+0x30),nodes,*geometry);
    }
}
void EmitNativeComplexMesh(uintptr_t hal,uintptr_t mesh,uintptr_t instances){auto& state=Owner(hal);
    if(!state.frame_open || !state.display_open || !state.programs || !state.shader_policy || !mesh || !instances)
        state.Fail("native_ui_complex_scope");
    if(!(Load<uint32_t>(hal+0x58)&0x20))return;
    const auto item=Load<uintptr_t>(mesh+0xA8);if(!item)return;
    const auto geometry=state.meshes->Acquire(item);
    if(!geometry || Load<uint32_t>(item+0x30)!=1)state.Fail("native_ui_complex_geometry");
    const auto fills=Load<uint64_t>(mesh+0xE8),records=Load<uintptr_t>(mesh+0xE0);
    const auto nodes=Load<uintptr_t>(instances),step=Load<uint64_t>(instances+0x10);
    const auto count=Load<uint32_t>(instances+8);
    if(!fills || fills>1024 || !records || count>4096 || (count && (!nodes || step<8 || step>4096)))
        state.Fail("native_ui_complex_draw_extent");
    for(uint64_t n=0;n<fills;++n){const auto record=records+n*0x38,fill=Load<uintptr_t>(record);
        const auto kind=Load<uint32_t>(fill+0x10);
        if(kind<1 || kind>15){char reason[96]{};sprintf_s(reason,"native_ui_complex_fill_%u_unavailable",kind);state.Fail(reason);}
        const uint32_t texture_count=kind<5?0u:(kind==9 || kind==10)?2u:1u;
        std::array<std::array<float,8>,2> uv{};
        if(texture_count){const auto matrices=Load<uintptr_t>(mesh+0xF8),matrix_count=Load<uint64_t>(mesh+0x100);
            if(!matrices || matrix_count>4096)state.Fail("native_ui_complex_uv_extent");
            for(uint32_t texture=0;texture<texture_count;++texture){const auto index=Load<uint32_t>(record+0x2C+texture*4);
                if(index>=matrix_count)state.Fail("native_ui_complex_uv_index");
                for(uint32_t value=0;value<8;++value)uv[texture][value]=Load<float>(matrices+uint64_t(index)*32+value*4);
            }
        }
        const auto format=Load<uintptr_t>(record+8);const auto stride=Load<uint32_t>(format);
        const auto vertex_offset=Load<uint64_t>(record+0x20),index_offset=uint64_t(Load<uint32_t>(record+0x18))*2;
        const auto vertices=Load<uint32_t>(record+0x28),indices=Load<uint32_t>(record+0x1C);
        const auto vertex_bytes=uint64_t(stride)*vertices,index_bytes=uint64_t(indices)*2;
        if(!stride || !vertices || !indices || vertex_offset>geometry->vertices.bytes.size() ||
           vertex_bytes>geometry->vertices.bytes.size()-vertex_offset || index_offset>geometry->indices.bytes.size() ||
           index_bytes>geometry->indices.bytes.size()-index_offset)state.Fail("native_ui_complex_fill_geometry_extent");
        PrimitiveGeometry slice;slice.incarnation=geometry->incarnation;slice.stride=stride;slice.vertex_count=vertices;slice.index_count=indices;
        const auto& vb=geometry->vertices.bytes;const auto& ib=geometry->indices.bytes;
        slice.vertices={1,std::vector<uint8_t>(vb.begin()+vertex_offset,vb.begin()+vertex_offset+vertex_bytes)};
        slice.indices={2,std::vector<uint8_t>(ib.begin()+index_offset,ib.begin()+index_offset+index_bytes)};
        for(size_t i=0;i<slice.indices.bytes.size();i+=2)
            if((uint32_t(slice.indices.bytes[i])|(uint32_t(slice.indices.bytes[i+1])<<8))>=vertices)
                state.Fail("native_ui_complex_fill_index_extent");
        for(uint32_t instance=0;instance<count;++instance){
            // Synchronous CPU field view: actual instance handle plus actual
            // geometry transform at complexMeshC0, read by the common 2D
            // algorithm through its normal-mesh B0 matrix offset. No object,
            // GPU slot or readiness is fabricated; no field view is retained.
            std::array<uintptr_t,2> current{Load<uintptr_t>(nodes+uint64_t(instance)*step),mesh+0x10};
            SubmitUiGeometry(hal,fill,format,reinterpret_cast<uintptr_t>(current.data()),slice,
                texture_count?reinterpret_cast<uintptr_t>(uv.data()):0,texture_count);
        }
    }
}
// ---- I-0067 SCALEFORM-FILTERS-NATIVE F2..F7: Scaleform filters (blur, drop shadow, glow) drawn natively. ----
// The original CPU code plans the passes (1F75440) and computes every uniform (1F75180, 1F78AD0, 1F770A0, 1F81EB0,
// 1D12410, 1C66270); the control flow of PushFilters (1BDC4E0), PopFilters (1BDB800), the pass loop (1F7B930), the
// uniform glue (1F778D0) and the CPU parts of Push/PopRenderTarget (1BDCBD0/1BDBB70) are transcribed below; draws are
// recorded into the frame packet with packet targets (F1) instead of D3D11 targets.
template<class F>F Original(const GameUiSession::State& state,uint32_t rva){return reinterpret_cast<F>(state.module+rva);}
struct FilterBindingScope {GameUiSession::State* state{};std::array<uint64_t,2> textures{};std::array<uint8_t,2> samplers{};uint32_t mask{};};
thread_local FilterBindingScope* filter_binding_scope{};
void FilterObjectUnsupported(uintptr_t) noexcept {
    if(filter_binding_scope)filter_binding_scope->state->Fail("native_ui_filter_object_method");std::terminate();}
uintptr_t FilterTargetTexture(FilterTarget* target) noexcept {return reinterpret_cast<uintptr_t>(&target->texture);}
// 1F81EB0 binds through texture vt+0x68(stage, sampler byte*): the stage is the pixel uniform row's register.
void BindFilterTexture(FilterTexture* texture,uint32_t stage,const uint8_t* sampler) noexcept {
    auto* scope=filter_binding_scope;if(!scope)std::terminate();
    if(stage>1 || (scope->mask&(1u<<stage)) || !sampler || *sampler>3 || !texture || !texture->sampled)scope->state->Fail("native_ui_filter_texture_bind");
    scope->textures[stage]=texture->sampled;scope->samplers[stage]=*sampler;scope->mask|=1u<<stage;
}
const std::array<uintptr_t,16> filter_target_table=[]{std::array<uintptr_t,16> t{};t.fill(reinterpret_cast<uintptr_t>(&FilterObjectUnsupported));
    t[0x18/8]=reinterpret_cast<uintptr_t>(&FilterTargetTexture);return t;}();
const std::array<uintptr_t,16> filter_texture_table=[]{std::array<uintptr_t,16> t{};t.fill(reinterpret_cast<uintptr_t>(&FilterObjectUnsupported));
    t[0x68/8]=reinterpret_cast<uintptr_t>(&BindFilterTexture);return t;}();
// The original temporary target (1F73750 through the texture manager) is a pooled render buffer of exactly the
// requested size, used whole: rectangle {0, 0, w, h}. No GPU object exists until a push gives it a packet use.
FilterTarget* CreateFilterTarget(GameUiSession::State& state,int32_t width,int32_t height){
    if(width<1 || height<1 || width>8192 || height>8192)return nullptr;
    auto target=std::make_unique<FilterTarget>();target->table=filter_target_table.data();
    target->buffer_width=static_cast<uint32_t>(width);target->buffer_height=static_cast<uint32_t>(height);target->rect={0,0,width,height};
    target->texture.table=filter_texture_table.data();target->texture.width=target->buffer_width;target->texture.height=target->buffer_height;
    state.filter_targets.push_back(std::move(target));return state.filter_targets.back().get();
}
// A genuine copy of the HAL matrix state (1C65EF0, copied through its vt+0x30 like the 0x340-byte target entries).
uintptr_t CopyMatrixState(const GameUiSession::State& state,uintptr_t source){
    const auto copy=reinterpret_cast<uintptr_t>(::operator new(0x2C0,std::align_val_t{16}));std::memset(reinterpret_cast<void*>(copy),0,0x2C0);
    Original<uintptr_t(*)(uintptr_t)>(state,0x1C65EF0)(copy);
    reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(Load<uintptr_t>(Load<uintptr_t>(copy)+0x30))(copy,source);return copy;
}
void ReleaseMatrixState(const GameUiSession::State& state,uintptr_t copy){
    if(const auto shared=Load<uintptr_t>(copy+0x288))Original<void(*)(uintptr_t)>(state,0x1BB6CA0)(shared);
    Original<void(*)(uintptr_t)>(state,0x1BB6AF0)(copy);::operator delete(reinterpret_cast<void*>(copy),std::align_val_t{16});
}
// PushRenderTarget(target, flags 8, rect, clear transparent), 1BDCBD0's CPU part: the viewport block, the matrix state
// (user matrix identity, clip offsets, identity blocks) and the target bits; the target's new packet use replaces
// the D3D11 bind and clear (vt+0x280). Masks and the stencil mode belong to the target left.
void PushFilterTarget(GameUiSession::State& state,uintptr_t hal,FilterTarget& target,const std::array<int32_t,4>& rect){
    if(state.target_stack.size()>=64)state.Fail("native_ui_filter_target_depth");
    GameUiSession::State::TargetScope scope;
    std::memcpy(scope.viewport.data(),reinterpret_cast<void*>(hal+0x1144),scope.viewport.size());
    const auto matrix=Load<uintptr_t>(hal+0x258);
    scope.flags=Load<uint32_t>(hal+0x58);scope.matrix=CopyMatrixState(state,matrix);
    scope.target=state.current_target;scope.width=state.current_width;scope.height=state.current_height;
    scope.masks=std::move(state.masks);state.masks.clear();
    scope.depth_mode=Load<uint32_t>(hal+0x1F8);scope.depth_reference=Load<uint32_t>(hal+0x1FC);scope.mask_clear_pending=std::exchange(state.mask_clear_pending,false);
    state.target_stack.push_back(std::move(scope));
    const auto old_left=Load<int32_t>(hal+0x1170),old_top=Load<int32_t>(hal+0x1174);
    Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)&~0x10u);
    for(size_t i=0;i<4;++i)Put<int32_t>(hal+0x1170+i*4,rect[i]);
    Put<int32_t>(hal+0x1144,static_cast<int32_t>(target.buffer_width));Put<int32_t>(hal+0x1148,static_cast<int32_t>(target.buffer_height));
    Put<int32_t>(hal+0x114C,target.rect[0]);Put<int32_t>(hal+0x1150,target.rect[1]);
    Put<int32_t>(hal+0x1154,target.rect[2]-target.rect[0]);Put<int32_t>(hal+0x1158,target.rect[3]-target.rect[1]);
    for(uintptr_t at=0x115C;at<=0x116C;at+=4)Put<int32_t>(hal+at,0);
    reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(Load<uintptr_t>(Load<uintptr_t>(matrix)+0x18))(matrix,state.module+0x454D740);
    Put<int32_t>(matrix+0x1C0,Load<int32_t>(matrix+0x1C0)-old_left);Put<int32_t>(matrix+0x1C8,Load<int32_t>(matrix+0x1C8)-old_left);
    Put<int32_t>(matrix+0x1C4,Load<int32_t>(matrix+0x1C4)-old_top);Put<int32_t>(matrix+0x1CC,Load<int32_t>(matrix+0x1CC)-old_top);
    const std::array<float,8> unit2x4{1,0,0,0,0,1,0,0};std::memcpy(reinterpret_cast<void*>(matrix+0x160),unit2x4.data(),32);
    const std::array<float,16> unit4x4{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};std::memcpy(reinterpret_cast<void*>(matrix+0x180),unit4x4.data(),64);
    Put<uint32_t>(hal+0x116C,Load<uint32_t>(hal+0x116C)|1u);Put<uint8_t>(matrix+0x280,1);
    Put<uint32_t>(hal+0x58,(Load<uint32_t>(hal+0x58)&~0x20u)|0x10u);
    // The new use: a packet target of the buffer's size and the texture that samples it.
    const auto use=state.frame_source.targets.size()+1;
    state.frame_source.targets.push_back({use,target.buffer_width,target.buffer_height});
    frame::PrimitiveTexture sampled{state.frame_source.textures.size()+1,{}};sampled.target=use;
    sampled.texture.width=target.buffer_width;sampled.texture.height=target.buffer_height;sampled.texture.format=DXGI_FORMAT_R8G8B8A8_UNORM;sampled.texture.mips.emplace_back();
    target.texture.use=use;target.texture.sampled=sampled.identity;state.frame_source.textures.push_back(std::move(sampled));
    state.current_target=use;state.current_width=target.buffer_width;state.current_height=target.buffer_height;
    Put<int32_t>(hal+0x280,Load<int32_t>(hal+0x280)+1);
    Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)|0x20u);
    Put<uint32_t>(hal+0x1F8,1);Put<uint32_t>(hal+0x1FC,0);
    NativeViewport(hal);
}
// PopRenderTarget, 1BDBB70's CPU part: the matrix state, viewport block and target bits of the target below.
void PopFilterTarget(GameUiSession::State& state,uintptr_t hal){
    if(state.target_stack.empty())state.Fail("native_ui_filter_target_underflow");
    auto scope=std::move(state.target_stack.back());state.target_stack.pop_back();
    const auto matrix=Load<uintptr_t>(hal+0x258);
    reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(Load<uintptr_t>(Load<uintptr_t>(matrix)+0x30))(matrix,scope.matrix);ReleaseMatrixState(state,scope.matrix);
    std::memcpy(reinterpret_cast<void*>(hal+0x1144),scope.viewport.data(),scope.viewport.size());
    state.current_target=scope.target;state.current_width=scope.width;state.current_height=scope.height;
    state.masks=std::move(scope.masks);state.mask_clear_pending=scope.mask_clear_pending;
    Put<uint32_t>(hal+0x1F8,scope.depth_mode);Put<uint32_t>(hal+0x1FC,scope.depth_reference);
    Put<uint32_t>(hal+0x58,(Load<uint32_t>(hal+0x58)&~0x30u)|(scope.flags&0x30u));
    if(scope.flags&0x20u)NativeViewport(hal);
}
// The filter quad (HAL+0x3B218, stride 12 at 143B4E144): two triangles over the unit square, batch index 0.
uint64_t FilterQuad(GameUiSession::State& state){
    if(state.filter_quad)return state.filter_quad;
    std::vector<uint8_t> bytes(72);const float corners[6][2]{{0,0},{1,0},{0,1},{0,1},{1,0},{1,1}};
    for(size_t i=0;i<6;++i)std::memcpy(bytes.data()+i*12,corners[i],8);
    state.filter_quad=state.frame_source.buffers.size()+1;state.frame_source.buffers.push_back({state.filter_quad,std::move(bytes)});
    return state.filter_quad;
}
using FilterTargets=std::array<FilterTarget*,3>;
// 1F778D0 for the blur family (kinds 0..5; only 0..2 are admitted): the same uniform writes in the same order, through
// the original writers, against the native shader-interface object. Its GPU commit (1F80640) is not run.
void FilterUniforms(const GameUiSession::State& state,uintptr_t hal,uintptr_t writer,uintptr_t metadata,const float* matrix,const float* cxform,
    const float* scale,uintptr_t filter,const FilterTargets& targets,const uint32_t* keys,uint32_t index,uint32_t count){
    using Write=void(*)(uintptr_t,uintptr_t,uint64_t,const void*,uint32_t,int32_t,int32_t);
    using Bind=void(*)(uintptr_t,uintptr_t,uint64_t,uintptr_t,const uint8_t*,int32_t);
    const auto write=[&](uint32_t uniform,const void* data,uint32_t values){Original<Write>(state,0x1F78AD0)(writer,metadata,uniform,data,values,0,0);};
    const auto bind=[&](uint32_t uniform,FilterTexture& texture,const uint8_t* sampler){
        Original<Bind>(state,0x1F81EB0)(writer,metadata,uniform,reinterpret_cast<uintptr_t>(&texture),sampler,0);};
    const uint8_t clamp_linear=3;
    write(0xD,matrix,8);
    const auto key=keys[index];const bool shadow=key-0x280000u<0x53002u;
    auto& source=*targets[0];auto& texture=source.texture;
    bind(0x14,texture,&clamp_linear);
    const float fy=static_cast<float>(source.rect[3]-source.rect[1])/static_cast<float>(texture.height);
    const float fx=static_cast<float>(source.rect[2]-source.rect[0])/static_cast<float>(texture.width);
    const std::array<float,8> uv{fx,fx*0.f,fx*0.f,(static_cast<float>(source.rect[0])+0.f)*fx,fy*0.f,fy,fy*0.f,(static_cast<float>(source.rect[1])+0.f)*fy};
    write(0x15,uv.data(),8);
    Original<void(*)(uintptr_t,uintptr_t,const float*,int32_t,int32_t)>(state,0x1F770A0)(writer,metadata,cxform,0,0);
    std::array<float,2> texel{1.f/static_cast<float>(texture.width),1.f/static_cast<float>(texture.height)};
    std::array<float,4> blur{};
    Original<void(*)(uintptr_t,uintptr_t,uint32_t,uint32_t,float*,const float*,float*,uint32_t)>(state,0x1F75180)
        (hal+0x11A0,filter+0x18,key,index,blur.data(),scale,texel.data(),count);
    write(8,blur.data(),4);write(0x16,texel.data(),2);
    if(shadow){std::array<float,4> colour{},second{};
        using Unpack=void(*)(uintptr_t,float*,float*,float*,float*);
        Original<Unpack>(state,0x1D12410)(filter+0x34,&colour[0],&colour[1],&colour[2],&colour[3]);
        Original<Unpack>(state,0x1D12410)(filter+0x38,&second[0],&second[1],&second[2],&second[3]);
        const std::array<float,2> offset{Load<float>(filter+0x28)*-0.05f*scale[0],Load<float>(filter+0x2C)*-0.05f*scale[5]};
        if(targets[2]){auto& retained=targets[2]->texture;
            const std::array<float,2> ratio{1.f/(static_cast<float>(retained.width)*texel[0]),1.f/(static_cast<float>(retained.height)*texel[1])};
            write(0x13,ratio.data(),2);bind(0x12,retained,&clamp_linear);}
        write(0xE,offset.data(),2);
        const auto kind=Load<int32_t>(filter+0x10);
        if(kind==3)write(0x11,second.data(),4);
        if(kind-4u<2u)state.Fail("native_ui_filter_gradient_unavailable");
        write(0x10,colour.data(),4);}
}
// 1F7AC90: one filter pass (uniforms, quad, 6 vertices) into the current target with the current blend and stencil; W1 F8a:
// also the cached quad of 1F7A590 (its own key and uniform writer, the same quad and state).
template<class Uniforms> void RecordUiQuadPass(GameUiSession::State& state,uintptr_t hal,uint32_t key,const Uniforms& uniforms){
    const auto id=++state.draw_identity,serial=state.frame_source.serial;
    FilterBindingScope bindings{&state};filter_binding_scope=&bindings;
    struct Reset {~Reset(){filter_binding_scope=nullptr;}} reset;
    UiCpuPreparedShader shader;
    const auto status=PrepareNativeUiFilterCpu(*state.programs,state.programs->Version(),serial,id,key,uniforms,shader,state.failure);
    if(status!=frame::PrimitiveConstantStatus::Ready){char reason[96]{};
        sprintf_s(reason,"native_ui_filter_pass_shader_status_%u_key_%X",static_cast<unsigned>(status),key);state.Fail(reason);}
    const auto& layout=shader.programs.vertex_layout;
    if(!layout.installed || !layout.supported || layout.elements.size()!=2 || layout.elements[0].format!=DXGI_FORMAT_R32G32_FLOAT ||
       layout.elements[0].byte_offset || layout.elements[1].format!=DXGI_FORMAT_R8_UINT || layout.elements[1].byte_offset!=8)
        state.Fail("native_ui_filter_vertex_layout");
    if(!(bindings.mask&1u))state.Fail("native_ui_filter_source_unbound");
    UiPrimitiveState policy;
    if(!DecodeUiPrimitiveState(Load<uint32_t>(hal+0x1F8),Load<uint32_t>(hal+0x1FC),Load<uint32_t>(hal+0x20C),
        Load<uint8_t>(hal+0x210),Load<uint8_t>(hal+0x211),CurrentUiViewport(hal),policy)){state.Skip("filter_state_family",Load<uint32_t>(hal+0x20C));return;}   // I-0101
    frame::PrimitiveDraw draw;draw.identity=id;
    draw.vertex_program=shader.programs.vertex.identity;draw.pixel_program=shader.programs.pixel.identity;
    AdmitFrameProgram(state,shader.programs.vertex);AdmitFrameProgram(state,shader.programs.pixel);
    draw.vertices=FilterQuad(state);draw.elements=layout.elements;draw.constants=shader.constants;draw.stride=12;draw.count=6;
    draw.viewport=policy.viewport;draw.blend=policy.blend;draw.stencil=policy.stencil;draw.stencil_reference=policy.stencil_reference;
    // The authored sampler objects (1F82AA0): bias -0.75, LOD 0..FLT_MAX.
    if(!DecodeUiSampler(bindings.samplers[0],draw.sampler))state.Fail("native_ui_filter_sampler");
    draw.texture=bindings.textures[0];draw.mip_bias=-.75f;draw.min_lod=0;draw.max_lod=FLT_MAX;
    if(bindings.mask&2u){frame::PrimitiveSampler second{};if(!DecodeUiSampler(bindings.samplers[1],second))state.Fail("native_ui_filter_sampler");
        draw.secondary={bindings.textures[1],second,-.75f,0,FLT_MAX};}
    ++state.filter_counts.passes;
    RecordUiDraw(state,hal,std::move(draw));
}
void RecordFilterPass(GameUiSession::State& state,uintptr_t hal,const FilterTargets& targets,uintptr_t filter,const uint32_t* keys,
    uint32_t index,uint32_t count,const float* matrix,const float* cxform){
    const auto scale=reinterpret_cast<const float*>((Load<uint32_t>(hal+0x54)&0x10000000u)?state.module+0x454D740:hal+0x1180);
    RecordUiQuadPass(state,hal,keys[index],[&](uintptr_t writer,uintptr_t metadata){
        FilterUniforms(state,hal,writer,metadata,matrix,cxform,scale,filter,targets,keys,index,count);});
}
// W1 F8a: the cached quad of a cacheAsBitmap primitive (1F7A590 lines 160-250, SCALEFORM-CACHE-COLORMATRIX 1): key 0x101
// plus 2 (plus 3 instead under a premultiplied blend, HAL+0x214 bit 0), plus 0x40 (bit 1), plus 8 (bit 0x10); the uniforms
// are the prefix of the filter writer: the composite matrix (0xD), the subtree target (0x14, clamp linear), its rectangle to
// UV (0x15) and the colour transform (1F770A0); none of the blur block, which the cache programmes do not declare.
void RecordCachedQuad(GameUiSession::State& state,uintptr_t hal,FilterTarget& source,const float* matrix,const float* cxform){
    const auto blend=Load<uint32_t>(hal+0x214);
    const uint32_t key=0x101u+((blend&1u)?3u:2u)+((blend&2u)?0x40u:0u)+((blend&0x10u)?8u:0u);
    const std::array<uint32_t,4> plan{key,source.texture.width,source.texture.height,0};
    if(state.cache_plans_seen.size()<16 && std::find(state.cache_plans_seen.begin(),state.cache_plans_seen.end(),plan)==state.cache_plans_seen.end()){
        state.cache_plans_seen.push_back(plan);char row[160]{};
        sprintf_s(row,"{\"event\":\"native_ui_cache_plan\",\"key\":\"%X\",\"target\":[%u,%u]}\n",key,source.texture.width,source.texture.height);state.filter_reports.emplace_back(row);}
    RecordUiQuadPass(state,hal,key,[&](uintptr_t writer,uintptr_t metadata){
        using Write=void(*)(uintptr_t,uintptr_t,uint64_t,const void*,uint32_t,int32_t,int32_t);
        using Bind=void(*)(uintptr_t,uintptr_t,uint64_t,uintptr_t,const uint8_t*,int32_t);
        const uint8_t clamp_linear=3;auto& texture=source.texture;
        Original<Write>(state,0x1F78AD0)(writer,metadata,0xD,matrix,8,0,0);
        Original<Bind>(state,0x1F81EB0)(writer,metadata,0x14,reinterpret_cast<uintptr_t>(&texture),&clamp_linear,0);
        const float fy=static_cast<float>(source.rect[3]-source.rect[1])/static_cast<float>(texture.height);
        const float fx=static_cast<float>(source.rect[2]-source.rect[0])/static_cast<float>(texture.width);
        const std::array<float,8> uv{fx,0.f,0.f,static_cast<float>(source.rect[0])*fx,0.f,fy,0.f,static_cast<float>(source.rect[1])*fy};
        Original<Write>(state,0x1F78AD0)(writer,metadata,0x15,uv.data(),8,0,0);
        Original<void(*)(uintptr_t,uintptr_t,const float*,int32_t,int32_t)>(state,0x1F770A0)(writer,metadata,cxform,0,0);});
    ++state.filter_counts.cached;
}
// The first child's 2x4 placement (byte table 1446E9BA4, stride 5): the filtered tree's bounds and composite matrix.
uintptr_t FilterChildPlacement(const GameUiSession::State& state,uintptr_t primitive){
    const auto child=Load<uintptr_t>(Load<uintptr_t>(Load<uintptr_t>(primitive+0x30)));
    return child+(uintptr_t(Load<uint8_t>(state.module+0x46E9BA4+uintptr_t(Load<uint8_t>(child+0xF)&0xF)*5))<<4)+0x10;
}
constexpr uint32_t filter_key_capacity=256;
// F2: 1F78E00 answers 1 at feature level 2 and above, for every primitive, the end markers (FilterSet nil) included;
// the render queue (1C64340) opens and closes the filter cache scope (slot +0x1C8) on that answer, so it must be the
// same for a begin and its end marker. Which filtered primitives run natively is decided separately at the push.
uint64_t NativeFilterCapabilities(uintptr_t hal,uintptr_t primitive){auto& state=Owner(hal);
    if(state.closed || !primitive)state.Fail("native_ui_filter_capability_scope");
    return 1;
}
// A primitive's filters run natively when no filter cache result is stored (+0x18, written only by 1C64F00 from the
// pass loop, the cached draw and PopBlendMode, all replaced here: always 0 in this HAL) and every filter is a blur,
// drop shadow or glow whose planned passes fit; otherwise the subtree is drawn without its filters.
bool NativeFiltersSupported(GameUiSession::State& state,uintptr_t hal,uintptr_t primitive){
    const auto filters=Load<uintptr_t>(primitive+0x48);
    if(!filters || Load<uint32_t>(primitive+0x18))return false;
    if(Load<uint32_t>(filters+0x18)>1024)state.Fail("native_ui_filter_capability_extent");
    const auto count=Original<uint32_t(*)(uintptr_t)>(state,0x1CC2AB0)(filters);
    // W1 F8a (SCALEFORM-CACHE-COLORMATRIX 5.1): an empty FilterSet is cacheAsBitmap: the subtree into a target, then one
    // cached quad (opt-in, ui-cache.flag; off keeps the former refusal: the subtree drawn in place).
    if(!count)return ui_cache.load(std::memory_order_relaxed);
    if(count>16)return false;
    std::array<uint32_t,filter_key_capacity> keys{};
    for(uint32_t i=0;i<count;++i){const auto filter=Original<uintptr_t(*)(uintptr_t,uint32_t)>(state,0x1CC2AA0)(filters,i);
        if(!filter || Load<uint32_t>(filter+0x10)>2 || Load<uint32_t>(filter+0x1C)>64 ||
           !std::isfinite(Load<float>(filter+0x20)) || !std::isfinite(Load<float>(filter+0x24)))return false;
        const auto passes=Original<uint32_t(*)(uintptr_t,uintptr_t,uint32_t,uint32_t*)>(state,0x1F75440)(hal+0x11A0,filter,Load<uint32_t>(hal+0x214),keys.data());
        if(!passes || passes>filter_key_capacity)return false;}
    return true;
}
// F3: PushFilters (1BDC4E0). The genuine HAL vector keeps {primitive, no target} in both branches.
void PushNativeFilterBody(GameUiSession::State& state,uintptr_t hal,uintptr_t primitive){
    const auto count=Load<uint64_t>(hal+0x180);
    if(!state.display_open || !primitive || count>=1024 || state.filter_scopes.size()!=count)state.Fail("native_ui_filter_push_scope");
    GameUiSession::State::FilterScope scope{primitive};
    // Review: an invisible display (HAL+0x58 bit 0x20 clear) records no draw, so its filters run nowhere.
    if((Load<uint32_t>(hal+0x58)&0x20u) && NativeFiltersSupported(state,hal,primitive) && !(Load<uint32_t>(hal+0x58)&0x300u)){
        const auto placement=FilterChildPlacement(state,primitive);
        const auto w=Load<float>(placement),h=Load<float>(placement+0x14),x=Load<float>(placement+0xC),y=Load<float>(placement+0x1C);
        FilterTarget* subtree{};
        // Refused (drawn without filters, counted): a rectangle that is empty after truncation (the projection needs
        // right > left) or a target beyond 4096 on a side (no allocation of that size for one UI element).
        if(std::isfinite(w) && std::isfinite(h) && std::isfinite(x) && std::isfinite(y) && std::fabs(x)+std::fabs(w)<1e9f && std::fabs(y)+std::fabs(h)<1e9f &&
           static_cast<int64_t>(x+w)-static_cast<int64_t>(x)>0 && static_cast<int64_t>(x+w)-static_cast<int64_t>(x)<=4096 &&
           static_cast<int64_t>(y+h)-static_cast<int64_t>(y)>0 && static_cast<int64_t>(y+h)-static_cast<int64_t>(y)<=4096 && w<4097.f && h<4097.f)
            subtree=CreateFilterTarget(state,static_cast<int32_t>(w),static_cast<int32_t>(h));
        if(subtree){
            scope.depth_mode=Load<uint32_t>(hal+0x1F8);scope.depth_reference=Load<uint32_t>(hal+0x1FC);scope.subtree=subtree;
            Put<uint32_t>(hal+0x1F8,1);Put<uint32_t>(hal+0x1FC,0);   // applyDepthStencilMode(1); raster 0 has no native state
            PushFilterTarget(state,hal,*subtree,{static_cast<int32_t>(x),static_cast<int32_t>(y),static_cast<int32_t>(x+w),static_cast<int32_t>(y+h)});
            Original<void(*)(uintptr_t,uint32_t,uint8_t,uint8_t)>(state,0x1BD9D90)(hal,1,0,1);
            Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)|0x80u);
            ++state.filter_counts.native;
        }else ++state.filter_counts.target_refusals;   // empty or unbounded subtree: drawn without its filters
    }else ++state.filter_counts.unfiltered;
    // CPU vector entry, as both original branches leave it; callers may inspect it between callbacks.
    Original<void(*)(uintptr_t,uintptr_t,uint64_t)>(state,0x1BDD980)(hal+0x178,hal+0x178,count+1);
    const auto entry=Load<uintptr_t>(hal+0x178)+count*0x10;
    Original<uint32_t(*)(uintptr_t)>(state,0x1BB6C10)(primitive);
    Put<uintptr_t>(entry,primitive);Put<uintptr_t>(entry+8,0);
    state.filter_scopes.push_back(scope);
}
// F6/F7: the pass loop (1F7B930) and the final composite into the target below.
void RunFilterPasses(GameUiSession::State& state,uintptr_t hal,const GameUiSession::State::FilterScope& scope){
    const auto filters=Load<uintptr_t>(scope.primitive+0x48);
    const auto count=Original<uint32_t(*)(uintptr_t)>(state,0x1CC2AB0)(filters);
    FilterTargets targets{scope.subtree,nullptr,nullptr};
    const int32_t width=scope.subtree->rect[2]-scope.subtree->rect[0],height=scope.subtree->rect[3]-scope.subtree->rect[1];
    Put<uint32_t>(hal+0x1F8,1);Put<uint32_t>(hal+0x1FC,0);
    Original<void(*)(uintptr_t,uint32_t,uint8_t,uint8_t)>(state,0x1BD9D90)(hal,0x12,1,0);
    const auto identity=reinterpret_cast<const float*>(state.module+0x46E91C0);
    const auto matrix_state=Load<uintptr_t>(hal+0x258);
    std::array<uint32_t,filter_key_capacity> keys{};
    uint32_t passes{},reached{};uintptr_t filter{};
    for(uint32_t f=0;f<count;++f){
        filter=Original<uintptr_t(*)(uintptr_t,uint32_t)>(state,0x1CC2AA0)(filters,f);
        passes=Original<uint32_t(*)(uintptr_t,uintptr_t,uint32_t,uint32_t*)>(state,0x1F75440)(hal+0x11A0,filter,Load<uint32_t>(hal+0x214),keys.data());
        if(!passes || passes>filter_key_capacity)state.Fail("native_ui_filter_plan_changed");
        // Trace (L1): each (kind, flags, passes, planned passes) once, with its fields and keys.
        const std::array<uint32_t,4> plan{Load<uint32_t>(filter+0x10),Load<uint32_t>(filter+0x18),Load<uint32_t>(filter+0x1C),passes};
        if(state.filter_plans_seen.size()<48 && std::find(state.filter_plans_seen.begin(),state.filter_plans_seen.end(),plan)==state.filter_plans_seen.end()){
            state.filter_plans_seen.push_back(plan);char row[640]{};
            int used=sprintf_s(row,"{\"event\":\"native_ui_filter_plan\",\"kind\":%u,\"flags\":%u,\"passes\":%u,\"blur\":[%g,%g],\"offset\":[%g,%g],\"strength\":%g,\"colour\":\"%08X\",\"target\":[%d,%d],\"planned\":%u,\"keys\":[",
                plan[0],plan[1],plan[2],Load<float>(filter+0x20),Load<float>(filter+0x24),Load<float>(filter+0x28),Load<float>(filter+0x2C),Load<float>(filter+0x30),
                Load<uint32_t>(filter+0x34),width,height,passes);
            for(uint32_t k=0;k<passes && k<16 && used>0 && used<560;++k)used+=sprintf_s(row+used,sizeof(row)-used,"%s\"%X\"",k?",":"",keys[k]);
            if(used>0 && used<600){sprintf_s(row+used,sizeof(row)-used,"]}\n");state.filter_reports.emplace_back(row);}}
        const bool retain=Load<int32_t>(filter+0x10)-1u<5u;
        if(retain)targets[2]=targets[0];
        bool failed{};uint32_t pass=0;
        for(;pass<passes;++pass){
            if(pass==passes-1 && f==count-1)break;
            if(!targets[1] && !(targets[1]=CreateFilterTarget(state,width,height))){
                ++state.filter_counts.target_refusals;targets[0]=scope.subtree;failed=true;break;}
            const auto& dest=*targets[1];
            PushFilterTarget(state,hal,*targets[1],dest.rect);
            std::array<float,8> matrix{};
            std::memcpy(matrix.data(),reinterpret_cast<const void*>(reinterpret_cast<uintptr_t(*)(uintptr_t,const int32_t*)>(
                Load<uintptr_t>(Load<uintptr_t>(matrix_state)+8))(matrix_state,reinterpret_cast<const int32_t*>(&dest.buffer_width))),32);
            RecordFilterPass(state,hal,targets,filter,keys.data(),pass,passes,matrix.data(),identity);
            const bool last=pass+1==passes-1 && f==count-1;
            if(retain && pass==0 && !last){
                if(!(targets[0]=CreateFilterTarget(state,width,height))){
                    ++state.filter_counts.target_refusals;PopFilterTarget(state,hal);targets[1]=nullptr;targets[0]=scope.subtree;failed=true;break;}}
            PopFilterTarget(state,hal);
            std::swap(targets[0],targets[1]);
        }
        reached=pass;
        if(failed)break;
    }
    // Non-persistent cache result (the original's store when the render buffer manager keeps no filter caches).
    Original<void(*)(uintptr_t,int32_t,uintptr_t,uint32_t)>(state,0x1C64F00)(scope.primitive,0,0,0);
    PopFilterTarget(state,hal);
    Put<uint32_t>(hal+0x1F8,scope.depth_mode);Put<uint32_t>(hal+0x1FC,scope.depth_reference);
    const auto placement=FilterChildPlacement(state,scope.primitive);
    std::array<float,8> composite{};
    Original<float*(*)(float*,uintptr_t,uintptr_t)>(state,0x1C66270)(composite.data(),matrix_state+0x1E0,placement);
    const auto child=Load<uintptr_t>(Load<uintptr_t>(Load<uintptr_t>(scope.primitive+0x30)));
    const auto flags=Load<uint8_t>(child+0xF);
    const auto cxform=(flags&1)?reinterpret_cast<const float*>(child+(uintptr_t(Load<uint8_t>(state.module+0x46E9BA0+uintptr_t(flags&0xF)*5))<<4)+0x10):identity;
    // 1BDEE90 reads the innermost blend primitive's mode (1 without one); this HAL keeps that stack in state.blends.
    const auto top=state.blends.empty()?1u:Load<uint32_t>(state.blends.back().primitive+0x48);
    Original<void(*)(uintptr_t,uint32_t,uint8_t,uint8_t)>(state,0x1BD9D90)(hal,top,1,1);
    if(count)RecordFilterPass(state,hal,targets,filter,keys.data(),reached,passes,composite.data(),cxform);
    else RecordCachedQuad(state,hal,*scope.subtree,composite.data(),cxform);   // W1 F8a: cacheAsBitmap
    const auto display=(Load<uint32_t>(hal+0x58)>>4)&1u;
    Original<void(*)(uintptr_t,uint32_t,uint8_t,uint8_t)>(state,0x1BD9D90)(hal,top,0,static_cast<uint8_t>(display));
    Put<int32_t>(hal+0x284,Load<int32_t>(hal+0x284)+static_cast<int32_t>(count));
}
// F3/F6: PopFilters (1BDB800).
void PopNativeFilterBody(GameUiSession::State& state,uintptr_t hal){
    const auto count=Load<uint64_t>(hal+0x180);
    if(!state.display_open || !count || state.filter_scopes.size()!=count)state.Fail("native_ui_filter_pop_scope");
    const auto top=Load<uintptr_t>(hal+0x178)+(count-1)*0x10;
    if(Load<uintptr_t>(top+8))state.Fail("native_ui_filter_target_not_owned");
    const auto scope=state.filter_scopes.back();state.filter_scopes.pop_back();
    if(Load<uintptr_t>(top)!=scope.primitive)state.Fail("native_ui_filter_scope_mismatch");
    std::array<uintptr_t,2> popped{};
    Original<void(*)(uintptr_t,void*)>(state,0x1BDB600)(hal+0x178,popped.data());
    if(scope.subtree){
        Put<int32_t>(hal+0x1138,-1);
        RunFilterPasses(state,hal,scope);
        if(!Load<uint64_t>(hal+0x180))Put<uint32_t>(hal+0x58,Load<uint32_t>(hal+0x58)&~0x80u);
    }
    if(popped[0])Original<void(*)(uintptr_t)>(state,0x1BB6CA0)(popped[0]);
}
// A field read failing inside the filter code would otherwise unwind through the engine's Scaleform frames.
void PushNativeFilter(uintptr_t hal,uintptr_t primitive){auto& state=Owner(hal);
    try{PushNativeFilterBody(state,hal,primitive);}catch(const std::exception& error){char reason[160]{};sprintf_s(reason,"native_ui_filter_push_exception %s",error.what());state.Fail(reason);}
}
void PopNativeFilter(uintptr_t hal){auto& state=Owner(hal);
    try{PopNativeFilterBody(state,hal);}catch(const std::exception& error){char reason[160]{};sprintf_s(reason,"native_ui_filter_pop_exception %s",error.what());state.Fail(reason);}
}
uintptr_t DestroyHal(uintptr_t hal,uint32_t){Owner(hal).Fail("native_ui_session_hal_external_destruction");}
uintptr_t DestroyWrapper(uintptr_t wrapper,uint32_t){Owner(Load<uintptr_t>(wrapper+8)).Fail("native_ui_session_wrapper_external_destruction");}
}
GameUiSession::GameUiSession(uintptr_t module,uintptr_t manager,TextureIngress& textures,DeviceQueues& gpu,
    std::mutex& submissions,uint64_t incarnation,Failure failure){
    if(!module || !manager || !failure || !gpu.device || !gpu.queue)throw std::invalid_argument("native UI session services");
    if(Load<uintptr_t>(manager)!=module+0x29909D8 || !Load<uintptr_t>(manager+0x968) ||
       Load<uintptr_t>(manager+0x978) || Load<uintptr_t>(manager+0x980) || Load<uintptr_t>(manager+0x988))
        throw std::runtime_error("native UI factory receiver not pristine");
    state_=std::make_unique<State>(module,manager,textures,gpu,submissions,incarnation,failure);auto& state=*state_;
    state.heap=Load<uintptr_t>(module+0x4936CC0);const auto heap_table=Load<uintptr_t>(state.heap);
    state.allocate=Load<uintptr_t>(heap_table+0x50);state.free=Load<uintptr_t>(heap_table+0x60);
    try {
        //1F72580 is a CPU-only base + uniform-array constructor. The mixed
        //1F72680 device HAL and1F75920 GPU initializer are never invoked.
        state.wrapper=state.Allocate(0x10);
        reinterpret_cast<uintptr_t(*)(uintptr_t)>(module+0x1BDF900)(state.wrapper);
        state.wrapper_table={reinterpret_cast<uintptr_t>(&DestroyWrapper),module+0x1BDF970,module+0x1BDF920};
        Put<uintptr_t>(state.wrapper,reinterpret_cast<uintptr_t>(state.wrapper_table.data()));
        state.hal=state.Allocate(0x3B5C0);
        reinterpret_cast<uintptr_t(*)(uintptr_t,uintptr_t)>(module+0x1F72580)(state.hal,state.wrapper);state.cpu_constructed=true;
        Put<uintptr_t>(state.wrapper+8,state.hal);Put<uint32_t>(state.hal+0x50,GetCurrentThreadId());
        DefaultMethods(state,std::make_index_sequence<128>{});
        // Recovered pure CPU getters retain their actual engine layouts. The
        //secondary StateBag at+10 remains the genuine CPU constructor's bag.
        for(const auto [slot,rva]:std::array<std::pair<size_t,uint32_t>,20>{{
            {0x98,0x1BDAFA0},{0xA0,0x1BDB1F0},{0xA8,0x1BDB2B0},{0xB0,0x1BDB280},
            {0xB8,0x1BDB270},{0xC0,0x1BDB290},{0xC8,0x1BDB2A0},{0xD8,0x1BDB260},
            {0xE0,0x1BDB140},{0xE8,0x1BDB230},{0xF0,0x1BDB220},{0xF8,0x1BDB330},
            {0x100,0x1BDB1D0},{0x108,0x1BDB1E0},{0x110,0x1BDB1C0},{0x120,0x1BDB2D0},
            {0x138,0x1BDB150},{0x140,0x1BDB390},{0x148,0x1BDB360},{0x160,0x1BDB0A0}}})state.table[slot/8]=module+rva;
        state.table[0]=reinterpret_cast<uintptr_t>(&DestroyHal);
        state.table[0x30/8]=reinterpret_cast<uintptr_t>(&ImageServicesAvailable);
        state.table[0x128/8]=reinterpret_cast<uintptr_t>(&ImageManager);
        state.table[0x130/8]=module+0x1F75730;
        state.table[0x58/8]=reinterpret_cast<uintptr_t>(&BeginCpuFrame);
        state.table[0x60/8]=reinterpret_cast<uintptr_t>(&EndCpuFrame);
        state.table[0x250/8]=reinterpret_cast<uintptr_t>(&BeginCpuDisplay);
        state.table[0x258/8]=reinterpret_cast<uintptr_t>(&EndCpuDisplay);
        state.table[0x260/8]=reinterpret_cast<uintptr_t>(&NativeViewport);
        state.table[0x268/8]=module+0x1BDE4F0;
        state.table[0x2D0/8]=module+0x1BDE4E0;
        // Genuine CPU render-tree traversal. Its nodes emit semantic calls
        // through this native HAL; unknown draw families still refuse here.
        state.table[0x188/8]=module+0x1BDA7A0;
        // CPU16-byte queue records and actual prepare/emit callbacks. Queue
        // consumers receive the native HAL; no old device command is replayed.
        state.table[0x88/8]=module+0x1BDA700;
        state.table[0x230/8]=reinterpret_cast<uintptr_t>(&MapCpuVertexFormat);
        state.table[0x238/8]=reinterpret_cast<uintptr_t>(&NativeBatchCapacity);
        state.table[0x240/8]=reinterpret_cast<uintptr_t>(&NativeFilterCapabilities);
        state.table[0x248/8]=module+0x1BDDEE0;
        state.table[0x190/8]=reinterpret_cast<uintptr_t>(&EmitNativePrimitive);
        state.table[0x198/8]=reinterpret_cast<uintptr_t>(&EmitNativeComplexMesh);
        state.table[0x1A0/8]=reinterpret_cast<uintptr_t>(&PushNativeMask);
        state.table[0x1A8/8]=reinterpret_cast<uintptr_t>(&EndNativeMask);
        state.table[0x1B0/8]=reinterpret_cast<uintptr_t>(&PopNativeMask);
        state.table[0x1B8/8]=reinterpret_cast<uintptr_t>(&PushNativeBlend);
        state.table[0x1C0/8]=reinterpret_cast<uintptr_t>(&PopNativeBlend);
        state.table[0x1C8/8]=reinterpret_cast<uintptr_t>(&NativeCacheScope);
        state.table[0x1D0/8]=reinterpret_cast<uintptr_t>(&PushNativeFilter);
        state.table[0x1D8/8]=reinterpret_cast<uintptr_t>(&PopNativeFilter);
        state.table[0x1E0/8]=reinterpret_cast<uintptr_t>(&PushCpuUserData);
        state.table[0x1E8/8]=reinterpret_cast<uintptr_t>(&PopCpuUserData);
        state.table[0x1F0/8]=module+0x1BDA8E0;
        state.table[0x288/8]=reinterpret_cast<uintptr_t>(&NativeBlendState);
        {const std::lock_guard<std::mutex> lock(registry_mutex);sessions.emplace(state.hal,&state);}
        Put<uintptr_t>(state.hal,reinterpret_cast<uintptr_t>(state.table.data()));
        // Pure CPU services extracted from the mixed initializer: the real
        //16-byte factory and its2C0-byte shared matrix/clip state. Movie capture
        //1BDA3A0 writes its current viewport here; no shader/GPU state exists.
        state.matrix_factory=state.Allocate(0x10);
        reinterpret_cast<uintptr_t(*)(uintptr_t,uintptr_t)>(module+0x1C66250)(state.matrix_factory,state.heap);
        Put<uintptr_t>(state.hal+0x250,state.matrix_factory);
        const auto matrices=reinterpret_cast<uintptr_t(*)(uintptr_t)>(module+0x1C66BB0)(state.matrix_factory);
        if(!matrices)throw std::bad_alloc();Put<uintptr_t>(state.hal+0x258,matrices);
        UiMeshCacheConfiguration config{{0x300000,0x1000000,0x300000,0xA00000,0x200000,0x40000},{0x18,5,0x2000,0x4000,0x1800}};
        reinterpret_cast<uintptr_t(*)(uintptr_t,uintptr_t,const UiMeshCacheConfiguration*)>(module+0x1F7C8E0)(state.hal+0x3AFF0,state.heap,&config);
        state.meshes->AdoptCache(reinterpret_cast<void*>(state.hal+0x3AFF0));
        // Genuine CPU fill queue storage, independently sized for this native
        // owner. Its16-byte records are not original GPU command packets.
        if(!reinterpret_cast<uint8_t(*)(uintptr_t,uint64_t)>(module+0x1C65B50)(state.hal+0xFA8,512))
            throw std::bad_alloc();
        CpuUiShapeTolerances tolerances{};std::memcpy(&tolerances,reinterpret_cast<void*>(state.hal+0x60),sizeof(tolerances));state.meshes->SetShapeTolerances(tolerances);
        // Real cache + real inline FillManager + real CPU queue/listener.
        const auto cache=Load<uintptr_t>(state.hal+0x248);
        reinterpret_cast<void(*)(uintptr_t,uintptr_t,uintptr_t)>(module+0x1C702E0)(cache,state.hal,state.hal+0x218);state.glyph_initialized=true;
        if(Load<uint32_t>(cache+0xFE0)!=1 || !Load<uintptr_t>(cache+0xF0))throw std::runtime_error("native session glyph initializer");
        Put<uintptr_t>(manager+0x978,state.wrapper);Put<uintptr_t>(manager+0x980,state.hal);
        // E527B0 performs CPU loader/font/file-opener registration, using our
        //128 callback for images. No old GPU factory enters this transaction.
        state.loader=reinterpret_cast<uintptr_t(*)(uintptr_t)>(module+0xE527B0)(manager);
        if(!state.loader)throw std::runtime_error("native session real loader unavailable");
        Put<uintptr_t>(manager+0x988,state.loader);
    }catch(...){Close();throw;}
}
GameUiSession::~GameUiSession(){Close();}
uintptr_t GameUiSession::Hal()const noexcept {return state_ && !state_->closed?state_->hal:0;}
uintptr_t GameUiSession::Loader()const noexcept {return state_ && !state_->closed?state_->loader:0;}
uintptr_t GameUiSession::TextureManager(){return state_ && !state_->closed?reinterpret_cast<uintptr_t>(state_->images.ConstructManager()):0;}
uintptr_t GameUiSession::ImportTexture(std::shared_ptr<const NativeTextureRecord> texture,const char** not_taken){
    if(!state_ || state_->closed){if(not_taken)*not_taken="owner_closed";return 0;}
    return state_->images.ImportTexture(std::move(texture),not_taken);}
uint32_t GameUiSession::PrepareFrame(const frame::PrimitiveProgramBank& programs,uint64_t serial,uint32_t width,uint32_t height){
    if(!state_ || state_->closed)return 0;auto& state=*state_;state.prepared.reset();
    const UiTimer total{0};   // D-0167 measurement (every call, counted in slot 5)
    if(ui_profiling.load(std::memory_order_relaxed))ui_profile[5].fetch_add(1,std::memory_order_relaxed);
    if(!serial || !width || !height)state.Fail("native_ui_frame_native_output");
    if(!state.shader_policy)state.shader_policy=std::make_unique<GameUiCpuShaderPolicy>(state.module,state.failure);
    state.shader_policy->Adopt();   // I-0088 P1: this frame's preparing thread
    state.programs=&programs;state.frame_source={};state.frame_textures.clear();state.frame_images.clear();
    if(!state.target_stack.empty() || !state.filter_scopes.empty())state.Fail("native_ui_frame_filter_scope");
    state.filter_targets.clear();state.filter_quad=0;state.current_target=0;state.current_width=state.current_height=0;
    state.filter_counts={};state.frame_source.content_incarnation=programs.Version().incarnation;
    state.frame_source.generation=state.frame_source.serial=serial;state.frame_source.texture_generation=state.images.Acquire()->version.generation;
    state.frame_source.width=width;state.frame_source.height=height;
    struct CpuLock {uintptr_t address;explicit CpuLock(uintptr_t p):address(p){
        reinterpret_cast<void(*)(uintptr_t,uint32_t)>(Load<uintptr_t>(Load<uintptr_t>(p)+0x10))(p,~0u);}
        ~CpuLock(){reinterpret_cast<void(*)(uintptr_t)>(Load<uintptr_t>(Load<uintptr_t>(address)+0x20))(address);}};
    std::vector<std::pair<int32_t,uintptr_t>> movies;
    // E5B940's actual authored-order selection, excluding its old target binds.
    {const CpuLock lock(state.manager+0xC38);const auto count=Load<uint64_t>(state.manager+0xAE0);
        if(count>32)state.Fail("native_ui_movie_frame_count");
        const auto begin=state.manager+0x9D8+((~(state.manager+0x9D8)+1)&7u);
        for(uint64_t n=0;n<count;++n){const auto movie=Load<uintptr_t>(begin+n*8);
            if(!movie)state.Fail("native_ui_movie_frame_null");
            if(Load<uint32_t>(movie+0x8C)){Put<uint32_t>(movie+0x8C,0);
                movies.emplace_back(Load<int32_t>(movie+0x20),movie);}}
    }
    if(movies.empty())return 0;
    std::stable_sort(movies.begin(),movies.end(),[](const auto& a,const auto& b){return a.first<b.first;});
    bool dark_logo_seen=false;   // D-0182
    {const UiTimer display{1};
    BeginCpuFrame(state.hal);
    for(const auto& [order,movie]:movies){(void)order;
        const bool dark_logo=!state.dark_logo_done && IsMovieNamed(movie,u"05_901_Logo_BNE");state.dark_logo_page=dark_logo;   // D-0182
        dark_logo_seen|=dark_logo;
        if(dark_logo){++state.dark_logo_calls;state.dark_logo_draw=state.dark_logo_displays=state.dark_logo_dimmed=state.dark_logo_imaged=state.dark_logo_restored=0;state.dark_logo_journal=false;}
        reinterpret_cast<void(*)(uintptr_t)>(state.module+0xE52EE0)(movie);
        state.dark_logo_page=false;if(dark_logo)DarkLogoPageReport(state);}
    EndCpuFrame(state.hal);}
    if(dark_logo_seen)state.dark_logo_absent=0;
    else if(state.dark_logo_calls && !state.dark_logo_done && ++state.dark_logo_absent>=60)DarkLogoRelease(state);   // D-0182
    // Trace (L6): the first 8 frames with filters, then every 300th: filtered primitives, passes, packet targets.
    if(const auto& c=state.filter_counts;c.native || c.unfiltered || c.target_refusals){const auto n=++state.filter_frames;
        if(n<=8 || !(n%300)){char row[320]{};
            sprintf_s(row,"{\"event\":\"native_ui_filter_frame\",\"frame\":%llu,\"native\":%llu,\"unfiltered\":%llu,\"refused_targets\":%llu,\"passes\":%llu,\"cached\":%llu,\"target_uses\":%zu,\"draws\":%zu}\n",
                static_cast<unsigned long long>(n),static_cast<unsigned long long>(c.native),static_cast<unsigned long long>(c.unfiltered),
                static_cast<unsigned long long>(c.target_refusals),static_cast<unsigned long long>(c.passes),static_cast<unsigned long long>(c.cached),state.frame_source.targets.size(),state.frame_source.draws.size());
            if(state.filter_reports.size()<256)state.filter_reports.emplace_back(row);}}
    if(!state.frame_source.draws.empty()){const UiTimer final_packet{3};frame::PrimitiveWorkPacket packet;
        // The frame source is complete (EndCpuFrame closed the scope); its payloads move into the admitted packet.
        if(frame::ProducePrimitiveWork(std::move(state.frame_source),packet).status!=frame::PrimitiveWorkStatus::Ready)state.Fail("native_ui_frame_packet_admission");
        state.frame_source={};state.frame_textures.clear();state.frame_images.clear();state.prepared=std::move(packet);}
    return static_cast<uint32_t>(movies.size());
}
const frame::PrimitiveWorkPacket* GameUiSession::PreparedFrame()const noexcept {
    return state_ && state_->prepared?&*state_->prepared:nullptr;
}
std::optional<frame::PrimitiveWorkPacket> GameUiSession::TakePreparedFrame() noexcept {
    if(!state_)return std::nullopt;return std::exchange(state_->prepared,std::nullopt);
}
std::vector<std::string> GameUiSession::TakeFilterReports(){
    if(!state_)return {};return std::exchange(state_->filter_reports,{});
}
void GameUiSession::Close(){if(!state_ || state_->closed)return;auto& state=*state_;const std::lock_guard<std::recursive_mutex> lock(state.mutex);
    // Withdraw the loader before the services it retains. E527B0's loader has
    // no shared count (loader+8 is a secondary pointer, RCA887); destroy it by
    // E586E0's actual contract: heap=EAA9D0(loader), virtual8(flags0), heap68.
    // A loader already withdrawn by the genuine destructor is not touched.
    if(state.loader){
        if(Load<uintptr_t>(state.manager+0x988)==state.loader){
            Put<uintptr_t>(state.manager+0x988,0);
            const auto heap=reinterpret_cast<uintptr_t(*)(uintptr_t)>(state.module+0xEAA9D0)(state.loader);
            if(!heap)state.Fail("native_ui_loader_heap_unavailable");
            reinterpret_cast<void(*)(uintptr_t,uint32_t)>(Load<uintptr_t>(Load<uintptr_t>(state.loader)+8))(state.loader,0);
            reinterpret_cast<void(*)(uintptr_t,uintptr_t)>(Load<uintptr_t>(Load<uintptr_t>(heap)+0x68))(heap,state.loader);
        }
        state.loader=0;
    }
    if(state.hal){
        state.cache_scopes.clear();
        for(const auto& scope:state.target_stack){ReleaseMatrixState(state,scope.matrix);   // and the masks of the targets below
            for(const auto& mask:scope.masks)reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BB6CA0)(mask.primitive);}
        state.target_stack.clear();state.filter_scopes.clear();state.filter_targets.clear();
        for(const auto& scope:state.blends)reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BB6CA0)(scope.primitive);
        state.blends.clear();
        for(const auto& scope:state.masks)reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BB6CA0)(scope.primitive);
        state.masks.clear();
        state.meshes.reset();
        if(state.cpu_constructed){const auto cache=Load<uintptr_t>(state.hal+0x248);
            if(cache){if(state.glyph_initialized)reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1C6E410)(cache);
                else reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BB6CA0)(cache);
                if(state.glyph_initialized)state.Free(cache);Put<uintptr_t>(state.hal+0x248,0);}
            if(Load<uint32_t>(state.hal+8)!=1)state.Fail("native_ui_session_hal_retained_on_close");
            reinterpret_cast<void(*)(uintptr_t)>(state.module+0x1BD9570)(state.hal);
        }
        // The base shared-ref teardown retires258. The factory250 has no
        //shared-ref prefix and remains this owner's actual CPU heap allocation.
        if(state.matrix_factory){Put<uintptr_t>(state.hal+0x250,0);state.Free(state.matrix_factory);state.matrix_factory=0;}
        {const std::lock_guard<std::mutex> registry_lock(registry_mutex);sessions.erase(state.hal);}
        Put<uintptr_t>(state.manager+0x980,0);state.Free(state.hal);state.hal=0;
    }
    if(state.wrapper){Put<uintptr_t>(state.manager+0x978,0);state.Free(state.wrapper);state.wrapper=0;}
    state.images.Close();state.closed=true;
}
}
namespace ds3native::bootstrap {
void GameUiSession::SetProfiling(bool enabled) noexcept {ui_profiling.store(enabled,std::memory_order_relaxed);}
void GameUiSession::SetUiCache(bool enabled) noexcept {ui_cache.store(enabled,std::memory_order_relaxed);}
GameUiSession::Profile GameUiSession::TakeProfile() noexcept {
    return {ui_profile[0].exchange(0),ui_profile[1].exchange(0),ui_profile[2].exchange(0),ui_profile[3].exchange(0),ui_profile[4].exchange(0),ui_profile[5].exchange(0),
        ui_profile[6].exchange(0),ui_profile[7].exchange(0),ui_profile[8].exchange(0),ui_profile[9].exchange(0)};
}
}
