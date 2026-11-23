// W10 GL-3/GL-4 T14 (W10-NATIVE-PASSES 5): our glare stage against the original's chain, per zone. For each Reference run with the
// draws capture and the glare state (Reference1649 m37, 1650 m31, 1651 m50, 1652 m45), the plan is built natively from the library's
// state (glare_capture.h); the stage draws it with the programmes found in the executable by their pins, from the scene HDR the
// original's first glare draw read; then every surface the plan wrote up to the composite is compared, texel for texel, with the
// original's output of the corresponding draw. Exactness is required on the capture's adapter and driver; elsewhere (WARP, another
// GPU) the differences are reported. Arguments: warp|hardware, the executable, the run directories (skipped with a message without
// them). Also: no debug-layer message of severity WARNING or worse.
// I-0138 tranche 2: with the profile warp-ours or hardware-ours the stage draws the plans with our own programmes
// (post_glare_programs.h) and the same comparison with the original's captured draws is made; then a second stage draws each plan
// with the library's programmes on the same device and every surface (the output's included) must equal ours byte for byte, on
// any adapter. The same comparison is then made away from the captures: each zone's state at four other extents, from two
// synthetic scenes; a plan with a technique that has no pinned original fails the test.
// I-0138 tranche 8f: the shape-3 captures (i138-t8e-oracle-240, -900) too. Their afterimage reads the former frame's history, so the
// stage draws such a plan twice in one list: the first frame lays it out (the history cleared black, then its copy_to), the history
// surface is then overwritten with the original's captured history (the afterimage draw's input), and the second frame, at the same
// generation (no clear), is the one compared; after it the history must equal that frame's afterimage output (the copy).
#include "glare_capture.h"
#include "synthetic_scene.h"
#include "post_test_programmes.h"
#include "../renderer/glare_stage.h"
#include "../renderer/post_glare_programs.h"
#include "../world/yebis_pins.h"
#include "../world/yebis_shafts.h"
#include "../content/acquire.h"
#include <dxgi1_6.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <stdexcept>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
using ds3native::synthetic::SyntheticScene;   // I-0138 tranche 4: moved to synthetic_scene.h, unchanged
using ds3native::post_test::LibraryProgrammes;   // I-0138 tranche 4: the three stage tests' one copy (post_test_programmes.h)
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;}
void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);}
// Whether a later draw of the plan writes draw k's surface again (LS-3: the shafts' ping-pong pair holds the later draw's output).
bool Rewritten(const yebis::GlarePlan& plan,size_t k){for(size_t j=k+1;j<plan.draws.size();++j)if(plan.draws[j].target==plan.draws[k].target)return true;return false;}
}
int main(int argc,char** argv){try{
    Require(argc>=4,"usage: glare_stage_test warp|hardware[-ours] <DarkSoulsIII.exe> <Reference run directory>...");
    std::string profile=argv[1];const bool ours=profile.size()>5 && !profile.compare(profile.size()-5,5,"-ours");if(ours)profile.resize(profile.size()-5);
    const bool warp=profile=="warp";Require(warp || profile=="hardware","unknown profile");
    _putenv_s("DS3_EXE",argv[2]);const auto exe=glare_capture::FileBytes(argv[2]);yebis::GlareData d;
    if(exe.empty() || !glare_capture::Data(d)){std::printf("PASS glare stage %s: skipped, no executable\n",argv[1]);return 0;}
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"control factory");ComPtr<IDXGIAdapter1> adapter;
    if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"control WARP adapter");
    else{Check(factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter)),"control hardware adapter");
        DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);Require(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE),"no hardware adapter");}
    std::string this_adapter;{DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);LARGE_INTEGER driver{};(void)adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice),&driver);
        char text[96]{};std::snprintf(text,sizeof(text),"%04X %04X %016llX",desc.VendorId,desc.DeviceId,static_cast<unsigned long long>(driver.QuadPart));this_adapter=text;}
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"control device");
    const D3D12_COMMAND_QUEUE_DESC queue_desc{D3D12_COMMAND_LIST_TYPE_DIRECT};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)),"control queue");
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"control allocator");
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"control fence");UINT64 fence_value{};
    GlareStage stage,library_stage;std::string reason;Require(stage.Open(device.Get(),reason),reason.c_str());
    if(ours)Require(library_stage.Open(device.Get(),reason),reason.c_str());
    size_t zones{},surfaces{},exact{},against_library{},equal_library{};bool same_everywhere=!warp;
    size_t with_content{},with_rgb{},shaft_surfaces{},shaft_with_rgb{};   // tranche 8i (the step review's H1): the original's surfaces that are not all zero
    size_t away_plans{},away_surfaces{},away_equal{},away_refused{};std::set<std::string> away_unpinned;
    uint64_t generation{};bool refusal_checked{};size_t pre_alone{},histories{},standard_zones{};
    // One recording of a plan by a stage with a set of programmes, from a scene HDR (RGBA16F, tight rows): the surfaces of the
    // plan's first `count` draws, read back (tight rows; empty for a surface a later draw writes again).
    const auto draw_plan=[&](GlareStage& with,const GlarePrograms& programs,const yebis::GlarePlan& plan,const std::vector<uint8_t>& scene,UINT W,UINT H,
            const std::vector<std::vector<uint8_t>>* film_grain,size_t count,const std::vector<uint8_t>* history=nullptr,
            std::vector<uint8_t>* history_after=nullptr){
        Require(scene.size()==size_t(W)*H*8,"the scene's size");
        Check(allocator->Reset(),"allocator reset");ComPtr<ID3D12GraphicsCommandList> list;
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=W;desc.Height=H;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        ComPtr<ID3D12Resource> t0;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&t0)),"scene");
        const UINT scene_pitch=(W*8+255)/256*256;   // the footprint's row pitch (a width that is not a multiple of 32 texels)
        const auto upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(scene_pitch)*H,D3D12_RESOURCE_STATE_GENERIC_READ);
        {uint8_t* memory{};const D3D12_RANGE none{};Check(upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"scene map");
            for(UINT y=0;y<H;++y)std::memcpy(memory+size_t(y)*scene_pitch,scene.data()+size_t(y)*W*8,size_t(W)*8);upload->Unmap(0,nullptr);}
        D3D12_TEXTURE_COPY_LOCATION to{t0.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,1,scene_pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        Transition(list.Get(),t0.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        // The blocks and the vignette vertices in one upload buffer (256-aligned ranges).
        const bool seeded=history && plan.history>=0;   // tranche 8f: a second Record over the captured history
        const auto capacity=post_test::TestConstantBytes(plan.draws.size()*((plan.pre.empty()?1:3)+(seeded?1:0))+plan.pre.size()*4);   // a plan with pre draws: three Records, four RecordPre
        const auto constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,capacity,D3D12_RESOURCE_STATE_GENERIC_READ);
        uint8_t* constant_memory{};{const D3D12_RANGE none{};Check(constants->Map(0,&none,reinterpret_cast<void**>(&constant_memory)),"constants map");}
        // I-0138 tranche 8b: the library's programmes (the second stage, or the first without -ours) read the library's registers.
        post_test::TestConstants block_memory(constant_memory,constants->GetGPUVirtualAddress(),capacity,&with==&library_stage || !ours);
        const uint64_t laid=with.Generation();
        auto* output=with.Record(list.Get(),t0.Get(),++generation,plan,programs,block_memory.Sink(),reason,film_grain);
        Require(output!=nullptr,reason.c_str());
        Require(!seeded || with.Generation()!=laid,"a plan drawn over the original's history did not lay out on its first frame");
        Require(!with.NeedsLayout(plan),"the stage would relay out the plan it just drew (a relayout every frame)");
        std::vector<ComPtr<ID3D12Resource>> keep_pre;   // the pre draws' scene and depths, alive until the list has run
        if(seeded){const auto& hs=plan.surfaces[size_t(plan.history)];const UINT texel=hs.half_float?8u:4u;const UINT pitch=(hs.width*texel+255)/256*256;
            Require(history->size()==size_t(hs.width)*hs.height*texel,"the captured history's size");
            const auto up=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*hs.height,D3D12_RESOURCE_STATE_GENERIC_READ);
            {uint8_t* memory{};const D3D12_RANGE none{};Check(up->Map(0,&none,reinterpret_cast<void**>(&memory)),"history map");
                for(UINT y=0;y<hs.height;++y)std::memcpy(memory+size_t(y)*pitch,history->data()+size_t(y)*hs.width*texel,size_t(hs.width)*texel);up->Unmap(0,nullptr);}
            auto* h=with.Surface(size_t(plan.history));const uint64_t layout=with.Generation();
            D3D12_TEXTURE_COPY_LOCATION hto{h,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},hfrom{up.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
            hfrom.PlacedFootprint.Footprint={hs.half_float?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R11G11B10_FLOAT,hs.width,hs.height,1,pitch};
            Transition(list.Get(),h,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);list->CopyTextureRegion(&hto,0,0,0,&hfrom,nullptr);
            Transition(list.Get(),h,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);keep_pre.push_back(up);
            output=with.Record(list.Get(),t0.Get(),generation,plan,programs,block_memory.Sink(),reason,film_grain);
            Require(output!=nullptr,reason.c_str());Require(with.Generation()==layout && with.Surface(size_t(plan.history))==h,"the second frame relaid the plan out");}
        // I-0138 tranche 8e (tranche 8d's final review, M1): the same plan's pre draws alone (a shape-0 frame with the motion blur's
        // Alpha_R) keep the layout; a different pre draw still asks for one.
        if(!plan.pre.empty()){auto only=plan;only.draws.clear();only.surfaces.clear();only.output=-1;
            Require(!with.NeedsLayout(only),"a plan of the same pre draws alone would relay out the kept surfaces");
            auto other=only;other.pre.front().write_mask=uint8_t(other.pre.front().write_mask^1u);
            Require(with.NeedsLayout(other),"a plan of other pre draws alone would keep the former pre layout");
            // Tranche 8e's step reviews (M2, then the fixes' M1): frames in the renderer's order (scene_pass_frame: RecordPre, then Record,
            // the same generation). A pre-only frame at a new generation; a full frame at a new generation; a pre-only frame with
            // a new depth at the same generation (RecordPre renews the heap and writes the main tables from the layout's record, the
            // kept scene view); then a full frame with nothing changed, so that Record writes no table and draws from those. No new
            // layout, the same surfaces, and (read back below, -ours) the plan's surfaces as the zone's stage drew the plan alone. The pre
            // draws go to a scene of their own (pre_scene), the main draws read t0. Without -ours the blocks are spread into the library's
            // registers while this stage draws our programmes: there only the layout and generation checks mean anything.
            const auto held=[&]{std::vector<ID3D12Resource*> s;for(size_t i=0;i<plan.surfaces.size();++i)s.push_back(with.Surface(i));return s;};
            const auto before=held();const uint64_t layout=with.Generation();
            D3D12_HEAP_PROPERTIES heap_pre{};heap_pre.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC pre_desc{};pre_desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            pre_desc.Width=W;pre_desc.Height=H;pre_desc.DepthOrArraySize=pre_desc.MipLevels=1;pre_desc.SampleDesc.Count=1;
            pre_desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;pre_desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
            ComPtr<ID3D12Resource> pre_scene;Check(device->CreateCommittedResource(&heap_pre,D3D12_HEAP_FLAG_NONE,&pre_desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&pre_scene)),"pre scene");
            list->DiscardResource(pre_scene.Get(),nullptr);
            pre_desc.Format=DXGI_FORMAT_R32G8X24_TYPELESS;pre_desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;ComPtr<ID3D12Resource> pre_depth[2];
            for(auto& z:pre_depth){Check(device->CreateCommittedResource(&heap_pre,D3D12_HEAP_FLAG_NONE,&pre_desc,D3D12_RESOURCE_STATE_DEPTH_WRITE,nullptr,IID_PPV_ARGS(&z)),"pre depth");
                list->DiscardResource(z.Get(),nullptr);Transition(list.Get(),z.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
            const auto pre=[&](const yebis::GlarePlan& p,ID3D12Resource* z,uint64_t g,const char* what){
                Require(with.RecordPre(list.Get(),pre_scene.Get(),z,g,p,programs,block_memory.Sink(),reason,film_grain),reason.c_str());
                Require(with.Generation()==layout && held()==before,what);};
            const auto full=[&](uint64_t g,const char* what){output=with.Record(list.Get(),t0.Get(),g,plan,programs,block_memory.Sink(),reason,film_grain);
                Require(output!=nullptr,reason.c_str());Require(with.Generation()==layout && held()==before,what);};
            pre(only,pre_depth[0].Get(),++generation,"the pre draws alone at a new generation renewed the surfaces");
            pre(plan,pre_depth[0].Get(),++generation,"a full frame's pre draws after the pre draws alone renewed the surfaces");
            full(generation,"a full frame after the pre draws alone renewed the surfaces");
            pre(only,pre_depth[1].Get(),generation,"the pre draws alone with a new depth renewed the surfaces");
            pre(plan,pre_depth[1].Get(),generation,"a full frame's pre draws with nothing changed renewed the surfaces");
            full(generation,"a full frame with nothing changed renewed the surfaces");
            keep_pre.push_back(pre_scene);for(auto& z:pre_depth)keep_pre.push_back(z);
            ++pre_alone;}
        std::vector<ComPtr<ID3D12Resource>> readbacks(count);std::vector<UINT> pitches(count);
        for(size_t k=0;k<count;++k){if(Rewritten(plan,k))continue;const auto& s=plan.surfaces[size_t(plan.draws[k].target)];const UINT texel=s.half_float?8u:4u;const UINT pitch=(s.width*texel+255)/256*256;
            auto* surface=with.Surface(size_t(plan.draws[k].target));auto rb=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(pitch)*s.height,D3D12_RESOURCE_STATE_COPY_DEST);
            Transition(list.Get(),surface,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
            D3D12_TEXTURE_COPY_LOCATION dst{rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},src{surface,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
            dst.PlacedFootprint.Footprint={s.half_float?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R11G11B10_FLOAT,s.width,s.height,1,pitch};list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            Transition(list.Get(),surface,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);readbacks[k]=rb;pitches[k]=pitch;}
        ComPtr<ID3D12Resource> history_rb;UINT history_pitch{};
        if(seeded && history_after){const auto& hs=plan.surfaces[size_t(plan.history)];const UINT texel=hs.half_float?8u:4u;history_pitch=(hs.width*texel+255)/256*256;
            auto* h=with.Surface(size_t(plan.history));history_rb=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(history_pitch)*hs.height,D3D12_RESOURCE_STATE_COPY_DEST);
            Transition(list.Get(),h,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
            D3D12_TEXTURE_COPY_LOCATION dst{history_rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},src{h,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
            dst.PlacedFootprint.Footprint={hs.half_float?DXGI_FORMAT_R16G16B16A16_FLOAT:DXGI_FORMAT_R11G11B10_FLOAT,hs.width,hs.height,1,history_pitch};list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            Transition(list.Get(),h,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);}
        block_memory.Spread();Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        const HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(queue->Signal(fence.Get(),++fence_value),"control signal");Check(fence->SetEventOnCompletion(fence_value,event),"control wait");
        WaitForSingleObject(event,INFINITE);CloseHandle(event);
        std::vector<std::vector<uint8_t>> rows(count);
        for(size_t k=0;k<count;++k){if(!readbacks[k])continue;const auto& s=plan.surfaces[size_t(plan.draws[k].target)];const size_t row=size_t(s.width)*(s.half_float?8u:4u);
            uint8_t* memory{};const D3D12_RANGE read{0,SIZE_T(pitches[k])*s.height};Check(readbacks[k]->Map(0,&read,reinterpret_cast<void**>(&memory)),"readback map");
            rows[k].resize(row*s.height);for(UINT y=0;y<s.height;++y)std::memcpy(rows[k].data()+size_t(y)*row,memory+size_t(y)*pitches[k],row);
            const D3D12_RANGE none{};readbacks[k]->Unmap(0,&none);}
        if(history_rb){const auto& hs=plan.surfaces[size_t(plan.history)];const size_t row=size_t(hs.width)*(hs.half_float?8u:4u);
            uint8_t* memory{};const D3D12_RANGE read{0,SIZE_T(history_pitch)*hs.height};Check(history_rb->Map(0,&read,reinterpret_cast<void**>(&memory)),"history readback map");
            history_after->resize(row*hs.height);for(UINT y=0;y<hs.height;++y)std::memcpy(history_after->data()+size_t(y)*row,memory+size_t(y)*history_pitch,row);
            const D3D12_RANGE none{};history_rb->Unmap(0,&none);}
        return rows;};
    const auto our_programmes=[](const yebis::GlarePlan& plan){return post_test::OurProgrammes(plan);};
    for(int a=3;a<argc;++a){const std::string dir=argv[a];const std::string name=dir.substr(dir.find_last_of("/\\")+1);
        const auto state=glare_capture::Load(dir);const auto rows=glare_capture::DrawRows(dir);
        Require(state.Ok() && !rows.empty(),"a named zone capture has no state or draws");
        const auto in=glare_capture::Inputs(state);auto plan=yebis::BuildGlare(d,in);Require(plan.refused.empty(),"a zone's plan is refused");
        // I-0138 tranche 8b (the third step review's L1): a plan holding a word without a slot, in a main draw and with no pre draw,
        // is refused whole by RecordPre and by Record, before anything is recorded or laid out (the first zone's, before any drawing: the
        // stage still needs its layout: with the plan's own programmes, a layout that ran would have succeeded). Then the same with
        // the word in a pre draw (RecordPre refuses it too). Needs the executable and a zone capture, as every check of this test.
        if(!refusal_checked){refusal_checked=true;auto bad=plan;bad.pre.clear();bad.draws.back().unplaced=true;
            auto bad_pre=plan;bad_pre.pre.assign(1,plan.draws.front());bad_pre.pre.back().unplaced=true;
            Check(allocator->Reset(),"allocator reset");ComPtr<ID3D12GraphicsCommandList> list;
            Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");
            const auto any=Buffer(device.Get(),D3D12_HEAP_TYPE_DEFAULT,256,D3D12_RESOURCE_STATE_COMMON);const auto programs=our_programmes(plan);
            const GlareConstantSink sink=[](uint64_t){return GlareConstantMemory{};};std::string pre_reason,reason2,reason3;
            const bool pre=stage.RecordPre(list.Get(),any.Get(),any.Get(),generation+1,bad,programs,sink,pre_reason);
            const auto* output=stage.Record(list.Get(),any.Get(),generation+1,bad,programs,sink,reason2);
            const bool pre_word=stage.RecordPre(list.Get(),any.Get(),any.Get(),generation+1,bad_pre,programs,sink,reason3);
            const auto refused=[](const std::string& r){return r.rfind("glare word without a slot",0)==0;};
            Require(!pre && !output && !pre_word && refused(pre_reason) && refused(reason2) && refused(reason3) && stage.NeedsLayout(bad) &&
                stage.NeedsLayout(bad_pre),"a plan with a word without a slot is not refused whole");
            Check(list->Close(),"control list close");}
        // LS-3: a run with the shaft state (Reference1667-1669) appends the shafts: the merged draws are the captured ones from the
        // first source downsample to the 2x draw, every surface but the 2x draw's compared (the mask, the radial blurs, and the layers' composite on
        // GlareShape 3 (MadT{n+1}, tranche 8i: the capture with its shafts) or MadT2 on shape 1).
        std::vector<size_t> original;std::shared_ptr<const std::vector<std::vector<uint8_t>>> film_grain;
        const bool shafts=name.find("shaftdriver")!=std::string::npos || name.find("-shafts-")!=std::string::npos || name.find("-sun-")!=std::string::npos;   /* tranche 8i: and GlareShape 3 with its shafts */yebis::ShaftInputs shaft_in;
        if(shafts){size_t mask{};Require(glare_capture::ShaftInputsOf(dir,rows,shaft_in,mask),"a shaft run's state");
            std::string refused;Require(yebis::AppendShafts(plan,shaft_in,yebis::BuildShaft(shaft_in),refused),"the shafts are not appended");
            char grain[96]{};std::snprintf(grain,sizeof(grain),"/reference-draw-%02zu-t1.raw",mask);
            auto levels=yebis::FilmGrainLevels(glare_capture::FileBytes(dir+grain));Require(levels.size()==9,"the mask's film grain");
            film_grain=std::make_shared<const std::vector<std::vector<uint8_t>>>(std::move(levels));
            size_t bright=rows.size(),sources=0;for(size_t n=0;n<rows.size();++n)if(glare_capture::Text(rows[n],"pass").rfind("tech_BrightPass",0)==0){bright=n;break;}
            while(sources<plan.draws.size() && plan.draws[sources].kind==yebis::GlareKind::Downsample)++sources;
            Require(bright<rows.size() && bright>=sources && bright-sources+plan.draws.size()<=rows.size(),"the capture does not hold the merged draws");
            for(size_t k=0;k<plan.draws.size();++k)original.push_back(bright-sources+k);}
        else original=glare_capture::OriginalDraws(plan,rows);
        Require(original.size()==plan.draws.size(),"the capture does not hold the plan's draws");
        char file[96]{};std::snprintf(file,sizeof(file),"/reference-draw-%02zu-t0.raw",original[0]);const auto scene=glare_capture::FileBytes(dir+file);
        const UINT W=in.width,H=in.height;Require(scene.size()==size_t(W)*H*8,"the scene capture's size");
        std::string captured_adapter;{std::snprintf(file,sizeof(file),"/reference-draw-%02zu-adapter.txt",original[0]);std::ifstream f(dir+file);std::getline(f,captured_adapter);}
        same_everywhere=same_everywhere && captured_adapter==this_adapter;
        // Every surface the plan wrote up to the composite is compared with the capture; the last draw's, the output, is read back
        // too for the comparison with the library's programmes.
        const size_t compared=plan.draws.size()-1;
        GlarePrograms library;{std::string unpinned;Require(LibraryProgrammes(exe,plan,library,unpinned),"a glare technique without a pin");}
        // Tranche 8f: a plan with an afterimage reads the original's history, its afterimage draw's input on the history surface.
        std::vector<uint8_t> history,history_after;size_t after=plan.draws.size();
        for(size_t k=0;k<plan.draws.size();++k)if(plan.draws[k].copy_to>=0)after=k;
        if(in.shape==3)++standard_zones;
        if(plan.history>=0){Require(after<plan.draws.size() && plan.draws[after].copy_to==plan.history,"the history's draw");size_t slot=8;
            for(size_t i=0;i<8;++i)if(plan.draws[after].inputs[i]==plan.history)slot=i;
            Require(slot<8,"the afterimage reads no history");std::snprintf(file,sizeof(file),"/reference-draw-%02zu-t%zu.raw",original[after],slot);
            history=glare_capture::FileBytes(dir+file);Require(!history.empty(),"the original's history input");}
        const auto drawn=draw_plan(stage,ours?our_programmes(plan):library,plan,scene,W,H,film_grain.get(),plan.draws.size(),
            history.empty()?nullptr:&history,history.empty()?nullptr:&history_after);
        if(!history.empty()){Require(!drawn[after].empty() && history_after==drawn[after],"the afterimage's output was not copied into the history");++histories;}
        size_t zone_exact{},checked{},zone_shaft{},zone_shaft_rgb{};
        // Tranche 8i (H1): a surface of zeros equal to zeros proves nothing: each compared reference surface is counted as with content
        // (a channel not zero; RGBA16F: a zero's sign ignored) and by its RGB alone (RGBA16F: alpha left out). A zone named « -sun-
        // » (i138-t8i-oracle-sun-*, a capture whose light shafts carry light) must carry content in its shaft surfaces (the mask
        // and the radial blurs, not the layers' composite): all zero in RGB fails the run. i138-t8i-oracle-shafts-1250 (all zero) is not so named.
        for(size_t k=0;k<compared;++k){if(drawn[k].empty())continue;++checked;const auto& s=plan.surfaces[size_t(plan.draws[k].target)];const UINT texel=s.half_float?8u:4u;
            std::snprintf(file,sizeof(file),"/reference-draw-%02zu-output.raw",original[k]);const auto captured=glare_capture::FileBytes(dir+file);
            Require(captured.size()==drawn[k].size(),"a captured output's size");
            {bool any_content=false,rgb_content=false;
                for(size_t t=0;t<captured.size() && !(any_content && rgb_content);t+=texel)for(UINT c=0;c<texel;++c){uint8_t b=captured[t+c];if(texel==8 && (c&1))b&=0x7F;if(!b)continue;any_content=true;if(texel!=8 || c<6)rgb_content=true;}
                const auto& g=plan.draws[k];const bool shaft_surface=shafts && (g.technique.rfind("tech_LightShaftMask",0)==0 || g.technique.rfind("tech_TexCoordMadT1_S",0)==0);   // H1 of c7e0ac515's review: not the layers' composite
                if(any_content)++with_content;if(rgb_content)++with_rgb;
                if(shaft_surface){++zone_shaft;++shaft_surfaces;if(rgb_content){++zone_shaft_rgb;++shaft_with_rgb;}}}
            uint64_t differing{};for(size_t t=0;t<captured.size();t+=texel)if(std::memcmp(drawn[k].data()+t,captured.data()+t,texel))++differing;
            if(differing)std::printf("%s draw %zu %s %ux%u: %llu of %u texels differ\n",name.c_str(),original[k],plan.draws[k].technique.c_str(),s.width,s.height,(unsigned long long)differing,s.width*s.height);
            else ++zone_exact;}
        std::printf("zone %s: %zu of %zu surfaces equal the original's draws %zu-%zu\n",name.c_str(),zone_exact,checked,original.front(),original[compared-1]);
        if(name.find("-sun-")!=std::string::npos){Require(zone_shaft>0,("zone "+name+": no shaft surface among the compared surfaces, and the zone is declared as carrying content in them").c_str());
            Require(zone_shaft_rgb>0,("zone "+name+": the shaft draws' surfaces are all zero: the capture proves nothing on the shafts").c_str());}
        ++zones;surfaces+=checked;exact+=zone_exact;
        // I-0138 tranche 8e (tranche 8d's final review, M1): the first zone's plan with an Alpha_R pre draw (the motion blur's) through a
        // stage of its own; draw_plan then checks that the same pre draws alone keep that layout and that other pre draws ask for one.
        // Tranche 8f: on the first zone whose plan has no afterimage (its frames' output does not depend on the frames before: a
        // history-reading plan changes at each of the check's frames).
        if(!pre_alone && plan.history<0){GlareStage pre_stage;Require(pre_stage.Open(device.Get(),reason),reason.c_str());auto with_pre=plan;
            yebis::AlphaRInputs alpha_in;alpha_in.spaces=yebis::NearDepthSpaces(0.1f,1000.0f);with_pre.pre.push_back(yebis::AlphaRDraw(yebis::AlphaR(alpha_in)));
            const auto again=draw_plan(pre_stage,our_programmes(with_pre),with_pre,scene,W,H,film_grain.get(),ours?with_pre.draws.size():0);   // read back for -ours only (the comparison)
            Require(pre_alone==1,"the pre-alone check did not run");
            if(ours)Require(again==drawn,"the full plan drawn after its pre draws alone differs from the plan drawn alone");}
        if(!ours)continue;
        // I-0138 tranche 2: our programmes against the library's, the same plan through a second stage on this device: every surface
        // byte for byte, on any adapter.
        {const auto theirs=draw_plan(library_stage,library,plan,scene,W,H,film_grain.get(),plan.draws.size(),history.empty()?nullptr:&history);
            for(size_t k=0;k<plan.draws.size();++k){if(drawn[k].empty())continue;++against_library;
                if(drawn[k]==theirs[k])++equal_library;
                else std::printf("%s draw %zu %s: our programme's surface differs from the library's\n",name.c_str(),original[k],plan.draws[k].technique.c_str());}}
        // Away from the captures: the zone's state at other extents (a small window, an odd size, 16:10, 3840x2160), the shafts
        // appended at the plan's sizes as the frame does, two synthetic scenes each. A plan the builder refuses at an extent is
        // reported; one with a technique that has no pinned original is drawn by ours alone (no debug-layer message), named, and
        // fails the test at its end.
        static constexpr UINT extents[4][2]{{800,450},{1501,843},{1680,1050},{3840,2160}};
        for(const auto& extent:extents){auto sized=in;sized.width=extent[0];sized.height=extent[1];auto other=yebis::BuildGlare(d,sized);
            if(other.refused.empty() && shafts){auto s=shaft_in;int bright=-1;for(const auto& g:other.draws)if(g.kind==yebis::GlareKind::BrightPass){bright=g.target;break;}
                Require(bright>=0,"a plan without a bright pass");s.base_width=other.base_width;
                s.mask_source={other.surfaces[size_t(bright)].width,other.surfaces[size_t(bright)].height};
                s.shaft=s.output=s.pingpong[0]=s.pingpong[1]={other.base_width,other.base_height};
                std::string refused;if(!yebis::AppendShafts(other,s,yebis::BuildShaft(s),refused))other.refused="shafts: "+refused;}
            if(!other.refused.empty()){std::printf("%s at %ux%u: the plan is refused (%s)\n",name.c_str(),extent[0],extent[1],other.refused.c_str());++away_refused;continue;}
            const auto mine_programs=our_programmes(other);GlarePrograms their_programs;std::string unpinned;
            if(!LibraryProgrammes(exe,other,their_programs,unpinned)){
                (void)draw_plan(stage,mine_programs,other,SyntheticScene(extent[0],extent[1],false),extent[0],extent[1],film_grain.get(),0);
                std::printf("%s at %ux%u: %s has no pinned original: drawn by ours alone, not compared\n",name.c_str(),extent[0],extent[1],unpinned.c_str());
                away_unpinned.insert(unpinned);continue;}
            for(const bool special:{false,true}){const auto synthetic=SyntheticScene(extent[0],extent[1],special);
                const auto mine=draw_plan(stage,mine_programs,other,synthetic,extent[0],extent[1],film_grain.get(),other.draws.size());
                const auto theirs=draw_plan(library_stage,their_programs,other,synthetic,extent[0],extent[1],film_grain.get(),other.draws.size());
                for(size_t k=0;k<other.draws.size();++k){if(mine[k].empty())continue;++away_surfaces;
                    if(mine[k]==theirs[k])++away_equal;
                    else std::printf("%s at %ux%u%s draw %zu %s: our programme's surface differs from the library's\n",name.c_str(),extent[0],extent[1],special?" (special values)":"",k,other.draws[k].technique.c_str());}}
            ++away_plans;}}
    ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");uint64_t messages{};
    for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::fprintf(stderr,"%s\n",message->pDescription);}}
    Require(messages==0,"debug-layer messages");
    if(!zones){std::printf("PASS glare stage %s: skipped, no zone capture\n",argv[1]);return 0;}
    if(same_everywhere)Require(exact==surfaces,"the native glare chain differs from the original on the capture's adapter");
    if(ours){std::printf("our programmes against the library's on this device: %zu of %zu surfaces byte for byte\n",equal_library,against_library);
        Require(against_library && equal_library==against_library,"our glare programmes differ from the library's");
        std::string names;for(const auto& technique:away_unpinned)names+=(names.empty()?"":", ")+technique;
        std::printf("our programmes against the library's away from the captures (four extents, two synthetic scenes): %zu of %zu surfaces byte for byte in %zu plans; "
            "%zu plans refused by the builder; techniques without a pinned original, drawn by ours alone: %s\n",away_equal,away_surfaces,away_plans,away_refused,names.empty()?"none":names.c_str());
        Require(away_surfaces && away_equal==away_surfaces,"our glare programmes differ from the library's away from the captures");
        Require(away_unpinned.empty(),"a plan names a technique without a pinned original (pin it from the library's technique table: yebis_pins.h)");}
    Require(histories==standard_zones,"a shape-3 zone's plan was not drawn over the original's history");
    std::printf("glare stage: %zu plans' pre draws alone keep the layout (tranche 8e); %zu plans drawn over the original's afterimage history, copied after their draw (tranche 8f)\n",pre_alone,histories);
    std::printf("PASS glare stage %s: T14: the native plans drawn by our glare stage from the original's scene HDR: %zu of %zu surfaces at %zu zones equal the original's (%zu with content, %zu with RGB content; %zu shaft surfaces, %zu with RGB content: a surface of zeros equal to zeros proves nothing)%s; no debug-layer message\n",
        argv[1],exact,surfaces,zones,with_content,with_rgb,shaft_surfaces,shaft_with_rgb,same_everywhere?" (the capture's adapter and driver: exactness required)":" (another adapter, driver or WARP: reported)");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
