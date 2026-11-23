// W10 DF-3 T15 (W10-NATIVE-PASSES 9): our depth of field drawn by the glare stage against the original's draws, per zone. For each
// Reference run with the DoF state (Reference1676 m37, 1677 m31, 1678 m50), the plan and the DoF table are built natively from the
// captured camera record and globals (dof_capture.h); the stage draws the depth factor (RecordPre: into the scene whose alpha is cleared,
// from the captured depth's plane 0 and the native table) and the other nine draws (Record) with the programmes found in the executable
// by their pins, from the scene the original's draw 0 wrote; then the scene's alpha and every surface are compared, texel for texel,
// with the original's outputs of draws 0-9. Exactness is required on the capture's adapter and driver; elsewhere the differences are
// reported. Arguments: warp|hardware, the executable, the run directories. Also: no debug-layer message of severity WARNING or worse.
// I-0138 tranche 4: with the profile warp-ours or hardware-ours the stage draws the plans with our own programmes
// (post_glare_programs.h) and the same comparison with the original's captured draws is made; then a second stage draws each plan
// with the library's programmes on the same device and the scene's alpha and every surface must equal ours byte for byte, on any
// adapter. The same comparison is then made away from the captures: each zone's camera and globals at four other extents, from
// two synthetic scenes and a synthetic depth; and on plans made for the comparison alone, one for each of the composite's forty
// counts, whose levels are drawn by the taps' programmes at counts that hold 16, 20 and 25. A plan with a technique that has no
// pinned original fails the test.
#include "dof_capture.h"
#include "synthetic_scene.h"
#include "post_test_programmes.h"
#include "../renderer/glare_stage.h"
#include "../renderer/post_glare_programs.h"
#include "../world/yebis_pins.h"
#include "../content/acquire.h"
#include <dxgi1_6.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <stdexcept>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
using ds3native::post_test::LibraryProgrammes;using ds3native::post_test::OurProgrammes;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;}
void Transition(ID3D12GraphicsCommandList* list,ID3D12Resource* r,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
    D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);}
// A texture of the given format filled from bytes (rows packed) through an upload buffer; left in `after`.
ComPtr<ID3D12Resource> Texture(ID3D12Device* device,ID3D12GraphicsCommandList* list,std::vector<ComPtr<ID3D12Resource>>& keep,DXGI_FORMAT format,UINT w,UINT h,UINT texel,
    const uint8_t* bytes,D3D12_RESOURCE_STATES after){
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=w;desc.Height=h;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=format;
    if(format==DXGI_FORMAT_R16G16B16A16_FLOAT)desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    ComPtr<ID3D12Resource> t;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&t)),"texture");
    const UINT pitch=(w*texel+255)/256*256;const auto upload=Buffer(device,D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*h,D3D12_RESOURCE_STATE_GENERIC_READ);keep.push_back(upload);
    {uint8_t* memory{};const D3D12_RANGE none{};Check(upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"texture map");
        for(UINT y=0;y<h;++y)std::memcpy(memory+size_t(y)*pitch,bytes+size_t(y)*w*texel,size_t(w)*texel);upload->Unmap(0,nullptr);}
    D3D12_TEXTURE_COPY_LOCATION to{t.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    from.PlacedFootprint.Footprint={format,w,h,1,pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    Transition(list,t.Get(),D3D12_RESOURCE_STATE_COPY_DEST,after);return t;}
void Put(yebis::GlareDraw& g,uint32_t word,float value){yebis::PutGlareWord(g,word,value);}
// I-0138 tranche 4: a plan made for the comparison alone, so that every count of the composite and the taps' three high counts
// are drawn by both sets of programmes. max(back, front) level surfaces, each a weighted-taps draw of the scene (its count from a
// list that holds 16, 20 and 25; offsets within three texels, positive weights of sum 1), then the composite TS{back}_{front}
// over the scene, a remap of the scene's factor for each level. Not a plan the builder makes: its words are arbitrary, the
// block's layout is the builder's (yebis_dof.cpp).
// Part 4b: blurred, the composite CompositeBlurredLevels_T{back + 1} (front 0) over the same levels.
yebis::GlarePlan CompositePlan(int back,int front,UINT W,UINT H,bool blurred=false){
    yebis::GlarePlan plan;plan.width=W;plan.height=H;uint32_t seed=0x40f0u+uint32_t(back)*8u+uint32_t(front);
    const auto next=[&]{seed=seed*1664525u+1013904223u;return seed>>8;};const auto unit=[&]{return float(next()&0xFFFF)/65535.0f;};
    static constexpr int counts[7]{16,20,25,9,12,15,6};const int levels=std::max(back,front);
    for(int i=0;i<levels;++i){const UINT w=std::max(W>>(1+i/2),UINT(8)),h=std::max(H>>(1+i/2),UINT(8));plan.surfaces.push_back({w,h,true});
        yebis::GlareDraw g;g.kind=yebis::GlareKind::Downsample;const int n=counts[(i+back+front)%7];g.technique="tech_MadT1_S"+std::to_string(n);g.target=int8_t(i);
        g.inputs[0]=-1;g.address[0]=3;Put(g,20,1.0f);Put(g,21,-1.0f);
        std::vector<float> weights(static_cast<size_t>(n),0.0f);float sum=0.0f;for(auto& x:weights){x=0.25f+unit();sum+=x;}
        for(int t=0;t<n;++t){const uint32_t at=(t<8?136u:168u)+4u*uint32_t(t);Put(g,at,(unit()*6.0f-3.0f)/float(W));Put(g,at+1,(unit()*6.0f-3.0f)/float(H));
            for(uint32_t c=0;c<4;++c)Put(g,616+4*uint32_t(t)+c,weights[size_t(t)]/sum);}
        plan.draws.push_back(std::move(g));}
    plan.surfaces.push_back({W,H,true});const int8_t output=int8_t(plan.surfaces.size()-1);
    yebis::GlareDraw g;g.kind=yebis::GlareKind::Composite;g.technique=blurred?"tech_DepthOfField_CompositeBlurredLevels_T"+std::to_string(back+1):"tech_DepthOfField_CompositeBlurLevelsFrontBlur_TS"+std::to_string(back)+"_"+std::to_string(front);g.target=output;
    Put(g,20,1.0f);Put(g,21,-1.0f);g.inputs[0]=-1;g.point[0]=true;g.address[0]=3;
    for(int k=0;k<levels;++k){g.inputs[size_t(k)+1]=int8_t(k);g.address[size_t(k)+1]=3;const uint32_t at=76+4*uint32_t(k);Put(g,at,unit()*4.0f-2.0f);Put(g,at+1,unit()*2.0f-0.5f);}
    // TEXCOORD1 a little off TEXCOORD0 (the builder's are equal): a programme reading the scene or a level at the other
    // coordinate then differs from the library's (the step review's M1).
    g.texcoord1=true;g.uv1={{{0.03125f,0.96875f},{0.03125f,-1.03125f},{2.03125f,0.96875f}}};
    plan.draws.push_back(std::move(g));plan.output=output;return plan;}
}
int main(int argc,char** argv){try{
    Require(argc>=4,"usage: dof_stage_test warp|hardware[-ours] <DarkSoulsIII.exe> <Reference run directory>...");
    std::string profile=argv[1];const bool ours=profile.size()>5 && !profile.compare(profile.size()-5,5,"-ours");if(ours)profile.resize(profile.size()-5);
    const bool warp=profile=="warp";Require(warp || profile=="hardware","unknown profile");
    _putenv_s("DS3_EXE",argv[2]);const auto exe=glare_capture::FileBytes(argv[2]);dof_capture::Data data;
    if(exe.empty() || !dof_capture::Load(data)){std::printf("PASS dof stage %s: skipped, no executable\n",argv[1]);return 0;}
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
    size_t away_plans{},away_surfaces{},away_equal{},away_refused{},sweep_plans{},sweep_surfaces{},sweep_equal{};std::set<std::string> unpinned_names,sweep_techniques;
    uint64_t generation{};
    // One recording of a plan by a stage with a set of programmes: the pre draw (the depth factor into the scene's alpha, from the
    // depth's plane 0 and the table) when the plan has one, then its draws, from a scene (RGBA16F, tight rows). Returns, tight rows:
    // [0] the scene after the pre draw, [k + 1] draw k's target (empty when a later draw writes it again).
    const auto draw_plan=[&](GlareStage& with,const GlarePrograms& programs,const yebis::GlarePlan& plan,const std::vector<uint8_t>& scene,const std::vector<uint8_t>& plane0,
            const std::vector<float>* table){
        const UINT W=plan.width,H=plan.height;Require(scene.size()==size_t(W)*H*8,"the scene's size");
        Require(plan.pre.empty() || plane0.size()==size_t(W)*H*4,"the depth's size");
        Check(allocator->Reset(),"allocator reset");ComPtr<ID3D12GraphicsCommandList> list;
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");
        std::vector<ComPtr<ID3D12Resource>> keep;
        const auto t0=Texture(device.Get(),list.Get(),keep,DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,8,scene.data(),D3D12_RESOURCE_STATE_RENDER_TARGET);
        const auto capacity=post_test::TestConstantBytes(plan.draws.size()+plan.pre.size());
        const auto constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,capacity,D3D12_RESOURCE_STATE_GENERIC_READ);
        uint8_t* constant_memory{};{const D3D12_RANGE none{};Check(constants->Map(0,&none,reinterpret_cast<void**>(&constant_memory)),"constants map");}
        // I-0138 tranche 8b: the library's programmes (the second stage, or the first without -ours) read the library's registers.
        post_test::TestConstants block_memory(constant_memory,constants->GetGPUVirtualAddress(),capacity,&with==&library_stage || !ours);const auto sink=block_memory.Sink();
        ++generation;
        // The depth as the renderer holds it: R32G8X24_TYPELESS with depth-stencil use, plane 0 from the given words.
        ComPtr<ID3D12Resource> z;
        if(!plan.pre.empty()){D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            desc.Width=W;desc.Height=H;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R32G8X24_TYPELESS;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
            Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&z)),"depth");
            const UINT pitch=(W*4+255)/256*256;const auto upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*H,D3D12_RESOURCE_STATE_GENERIC_READ);keep.push_back(upload);
            uint8_t* memory{};const D3D12_RANGE none{};Check(upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"depth map");
            for(UINT y=0;y<H;++y)std::memcpy(memory+size_t(y)*pitch,plane0.data()+size_t(y)*W*4,size_t(W)*4);upload->Unmap(0,nullptr);
            D3D12_TEXTURE_COPY_LOCATION to{z.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
            from.PlacedFootprint.Footprint={DXGI_FORMAT_R32_TYPELESS,W,H,1,pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
            Transition(list.Get(),z.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Require(with.RecordPre(list.Get(),t0.Get(),z.Get(),generation,plan,programs,sink,reason,nullptr,table),reason.c_str());}
        Transition(list.Get(),t0.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        auto* output=with.Record(list.Get(),t0.Get(),generation,plan,programs,sink,reason);Require(output!=nullptr,reason.c_str());
        Require(!with.NeedsLayout(plan),"the stage would relay out the plan it just drew (a relayout every frame)");
        // Readbacks: the scene and each draw's target (a target written again later holds the later draw).
        struct Read {ComPtr<ID3D12Resource> buffer;UINT pitch{},width{},height{};size_t index{};};std::vector<Read> reads;
        const auto readback=[&](ID3D12Resource* r,UINT w,UINT h,size_t index){const UINT pitch=(w*8+255)/256*256;auto rb=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(pitch)*h,D3D12_RESOURCE_STATE_COPY_DEST);
            Transition(list.Get(),r,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
            D3D12_TEXTURE_COPY_LOCATION dst{rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},src{r,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
            dst.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,w,h,1,pitch};list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            Transition(list.Get(),r,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);reads.push_back({rb,pitch,w,h,index});};
        readback(t0.Get(),W,H,0);
        for(size_t k=0;k<plan.draws.size();++k){bool later=false;for(size_t j=k+1;j<plan.draws.size();++j)later=later || plan.draws[j].target==plan.draws[k].target;if(later)continue;
            const auto& s=plan.surfaces[size_t(plan.draws[k].target)];Require(s.half_float,"a DoF surface that is not RGBA16F");
            readback(with.Surface(size_t(plan.draws[k].target)),s.width,s.height,k+1);}
        block_memory.Spread();Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        const HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(queue->Signal(fence.Get(),++fence_value),"control signal");Check(fence->SetEventOnCompletion(fence_value,event),"control wait");
        WaitForSingleObject(event,INFINITE);CloseHandle(event);
        std::vector<std::vector<uint8_t>> rows(plan.draws.size()+1);
        for(const auto& r:reads){const size_t row=size_t(r.width)*8;uint8_t* memory{};const D3D12_RANGE range{0,SIZE_T(r.pitch)*r.height};
            Check(r.buffer->Map(0,&range,reinterpret_cast<void**>(&memory)),"readback map");
            rows[r.index].resize(row*r.height);for(UINT y=0;y<r.height;++y)std::memcpy(rows[r.index].data()+size_t(y)*row,memory+size_t(y)*r.pitch,row);
            const D3D12_RANGE none{};r.buffer->Unmap(0,&none);}
        return rows;};
    // Ours against the library's, a plan through both stages from the same inputs: the surfaces compared, the equal ones counted.
    const auto against=[&](const yebis::GlarePlan& plan,const GlarePrograms& mine,const GlarePrograms& theirs,const std::vector<uint8_t>& scene,const std::vector<uint8_t>& plane0,
            const std::vector<float>* table,const std::string& what,size_t& compared,size_t& equal){
        const auto a=draw_plan(stage,mine,plan,scene,plane0,table),b=draw_plan(library_stage,theirs,plan,scene,plane0,table);
        for(size_t i=0;i<a.size();++i){if(a[i].empty() || (i==0 && plan.pre.empty()))continue;++compared;
            if(a[i]==b[i])++equal;
            else{uint64_t differing{};for(size_t t=0;t<a[i].size();t+=8)differing+=std::memcmp(a[i].data()+t,b[i].data()+t,8)!=0;
                std::printf("%s %s %s: our programme's surface differs from the library's at %llu texels\n",what.c_str(),i?"draw":"the depth factor",
                    (i?plan.draws[i-1].technique:plan.pre[0].technique).c_str(),(unsigned long long)differing);}}};
    for(int a=3;a<argc;++a){const std::string dir=argv[a];const std::string name=dir.substr(dir.find_last_of("/\\")+1);
        dof_capture::Frame frame;std::string why;Require(dof_capture::Native(data,dir,frame,why),("a zone's native frame: "+why).c_str());
        const auto plan=yebis::BuildDof(data.draw,frame.frame);Require(plan.refused.empty(),("a zone's plan is refused: "+plan.refused).c_str());
        const UINT W=plan.width,H=plan.height;
        auto scene=glare_capture::FileBytes(dir+"/reference-draw-00-output.raw");const auto depth=glare_capture::FileBytes(dir+"/reference-draw-00-t0.raw");
        Require(scene.size()==size_t(W)*H*8 && depth.size()==size_t(W)*H*8,"the scene or depth capture's size");
        const auto original_scene=scene;for(size_t i=0;i<size_t(W)*H;++i){scene[i*8+6]=0;scene[i*8+7]=0;}   // the alpha draw 0 writes, cleared
        std::vector<uint8_t> plane0(size_t(W)*H*4);for(size_t i=0;i<size_t(W)*H;++i)std::memcpy(plane0.data()+i*4,depth.data()+i*8,4);
        std::string captured_adapter;{std::ifstream f(dir+"/reference-draw-00-adapter.txt");std::getline(f,captured_adapter);}
        same_everywhere=same_everywhere && captured_adapter==this_adapter;
        GlarePrograms library;{std::string unpinned;Require(LibraryProgrammes(exe,plan,library,unpinned),"a DoF technique without a pin");}
        const auto drawn=draw_plan(stage,ours?OurProgrammes(plan):library,plan,scene,plane0,&frame.table);
        size_t zone_exact{},checked{};
        for(size_t r=0;r<drawn.size();++r){if(drawn[r].empty())continue;++checked;char file[96]{};std::snprintf(file,sizeof(file),"/reference-draw-%02zu-output.raw",r);
            const auto captured=r?glare_capture::FileBytes(dir+file):original_scene;Require(captured.size()==drawn[r].size(),"a captured output's size");
            uint64_t differing{};for(size_t t=0;t<captured.size();t+=8)if(std::memcmp(drawn[r].data()+t,captured.data()+t,8))++differing;
            const std::string technique=r?plan.draws[r-1].technique:plan.pre[0].technique;const auto& s=r?plan.surfaces[size_t(plan.draws[r-1].target)]:yebis::GlareSurface{W,H,true};
            if(differing)std::printf("%s draw %zu %s %ux%u: %llu of %u texels differ\n",name.c_str(),r,technique.c_str(),s.width,s.height,(unsigned long long)differing,s.width*s.height);
            else ++zone_exact;}
        std::printf("zone %s: %zu of %zu surfaces equal the original's draws 0-%zu\n",name.c_str(),zone_exact,checked,plan.draws.size());
        ++zones;surfaces+=checked;exact+=zone_exact;
        if(!ours)continue;
        // I-0138 tranche 4: our programmes against the library's on this device, the capture's inputs.
        against(plan,OurProgrammes(plan),library,scene,plane0,&frame.table,name,against_library,equal_library);
        // Away from the captures: the zone's camera and globals at other extents (a small window, an odd size, 16:10, 3840x2160),
        // two synthetic scenes and a synthetic depth each. A frame or a plan the builders refuse at an extent is reported; a plan
        // with a technique that has no pinned original is named and fails the test at its end.
        static constexpr UINT extents[4][2]{{800,450},{1501,843},{1680,1050},{3840,2160}};
        for(const auto& extent:extents){dof_capture::Frame sized;std::string refused;
            if(!dof_capture::Native(data,dir,sized,refused,extent[0],extent[1])){std::printf("%s at %ux%u: the frame is refused (%s)\n",name.c_str(),extent[0],extent[1],refused.c_str());++away_refused;continue;}
            const auto other=yebis::BuildDof(data.draw,sized.frame);
            if(!other.refused.empty()){std::printf("%s at %ux%u: the plan is refused (%s)\n",name.c_str(),extent[0],extent[1],other.refused.c_str());++away_refused;continue;}
            GlarePrograms theirs;std::string unpinned;
            if(!LibraryProgrammes(exe,other,theirs,unpinned)){std::printf("%s at %ux%u: %s has no pinned original: not compared\n",name.c_str(),extent[0],extent[1],unpinned.c_str());unpinned_names.insert(unpinned);continue;}
            const auto mine=OurProgrammes(other);const auto synthetic_depth=synthetic::SyntheticDepth(extent[0],extent[1]);
            char what[160]{};std::snprintf(what,sizeof(what),"%s at %ux%u",name.c_str(),extent[0],extent[1]);
            for(const bool special:{false,true})against(other,mine,theirs,synthetic::SyntheticScene(extent[0],extent[1],special),synthetic_depth,&sized.table,
                std::string(what)+(special?" (special values)":""),away_surfaces,away_equal);
            ++away_plans;}}
    if(ours && zones){
        // The composite's forty counts and the taps' high counts, on plans made for the comparison (CompositePlan), two synthetic scenes.
        for(int back=0;back<=4;++back)for(int front=0;front<=7;++front){const auto plan=CompositePlan(back,front,320,180);
            GlarePrograms theirs;std::string unpinned;
            if(!LibraryProgrammes(exe,plan,theirs,unpinned)){std::printf("the composite's plan %d %d: %s has no pinned original: not compared\n",back,front,unpinned.c_str());unpinned_names.insert(unpinned);continue;}
            const auto mine=OurProgrammes(plan);for(const auto& g:plan.draws)sweep_techniques.insert(g.technique);
            char what[96]{};std::snprintf(what,sizeof(what),"the composite's plan %d %d",back,front);
            for(const bool special:{false,true})against(plan,mine,theirs,synthetic::SyntheticScene(320,180,special),{},nullptr,std::string(what)+(special?" (special values)":""),sweep_surfaces,sweep_equal);
            ++sweep_plans;}
        // Part 4b: CompositeBlurredLevels_T1 to T8 on the same kind of plans (back n - 1 levels, the scene's factor taken absolute).
        for(int n=1;n<=8;++n){const auto plan=CompositePlan(n-1,0,320,180,true);
            GlarePrograms theirs;std::string unpinned;
            if(!LibraryProgrammes(exe,plan,theirs,unpinned)){std::printf("the T%d composite's plan: %s has no pinned original: not compared\n",n,unpinned.c_str());unpinned_names.insert(unpinned);continue;}
            const auto mine=OurProgrammes(plan);for(const auto& g:plan.draws)sweep_techniques.insert(g.technique);
            char what[96]{};std::snprintf(what,sizeof(what),"the T%d composite's plan",n);
            for(const bool special:{false,true})against(plan,mine,theirs,synthetic::SyntheticScene(320,180,special),{},nullptr,std::string(what)+(special?" (special values)":""),sweep_surfaces,sweep_equal);
            ++sweep_plans;}}
    ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");uint64_t messages{};
    for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::fprintf(stderr,"%s\n",message->pDescription);}}
    Require(messages==0,"debug-layer messages");
    if(!zones){std::printf("PASS dof stage %s: skipped, no zone capture\n",argv[1]);return 0;}
    if(same_everywhere)Require(exact==surfaces,"the native depth of field differs from the original on the capture's adapter");
    if(ours){std::printf("our programmes against the library's on this device: %zu of %zu surfaces byte for byte\n",equal_library,against_library);
        Require(against_library && equal_library==against_library,"our DoF programmes differ from the library's");
        std::string names;for(const auto& technique:unpinned_names)names+=(names.empty()?"":", ")+technique;
        std::printf("our programmes against the library's away from the captures (four extents, two synthetic scenes, a synthetic depth): %zu of %zu surfaces byte for byte in %zu plans; "
            "%zu frames or plans refused by the builders; techniques without a pinned original: %s\n",away_equal,away_surfaces,away_plans,away_refused,names.empty()?"none":names.c_str());
        Require(away_surfaces && away_equal==away_surfaces,"our DoF programmes differ from the library's away from the captures");
        std::printf("our programmes against the library's on the composite's forty counts and the eight T{n} (plans made for the comparison, two synthetic scenes): %zu of %zu surfaces byte for byte in %zu plans, %zu techniques\n",
            sweep_equal,sweep_surfaces,sweep_plans,sweep_techniques.size());
        Require(sweep_plans==48 && sweep_equal==sweep_surfaces,"our DoF programmes differ from the library's on the composite's counts");
        for(const char* technique:{"tech_MadT1_S16","tech_MadT1_S20","tech_MadT1_S25"})Require(sweep_techniques.contains(technique),"a taps count of the comparison's plans was not drawn");
        Require(unpinned_names.empty(),"a plan names a technique without a pinned original (pin it from the library's technique table: yebis_pins.h)");}
    std::printf("PASS dof stage %s: T15: the native DoF plans drawn by the glare stage from the original's scene and depth: %zu of %zu surfaces at %zu zones equal the original's%s; no debug-layer message\n",
        argv[1],exact,surfaces,zones,same_everywhere?" (the capture's adapter and driver: exactness required)":" (another adapter, driver or WARP: reported)");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
