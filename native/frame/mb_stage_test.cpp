// W10 MB-2 T16 (W10-NATIVE-PASSES 10): our motion blur drawn by the glare stage against the original's draws, per zone. For each
// Reference run with the library at the Alpha_R draw (Reference1676 m37, 1677 m31, 1678 m50), the plan is yebis::BuildMotionBlur of
// the library's fields (mb_capture.h); the stage draws the camera velocity from the captured depth (its plane 0, as the renderer holds
// it) and object velocity (draw 11's t0 and t1), then the blur passes from the captured scene (draw 12's t0) with our own internal
// velocity, with the programmes found in the executable by their pins; each surface is compared texel for texel with the original's
// outputs of draws 11-13. The velocity surface may differ by one half-float unit in its red and green channels (MB-0: INFERRED, the D3D11
// and D3D12 compilations of the same programme); chained after it, the blur passes may differ on at most 0.1% of the texels on the capture's
// adapter (Reference1693, a turning camera: 47 and 228 of 921600), and at one run at least the original's blur must change texels (the
// still-camera captures 1676-1678 change none). Arguments:
// warp|hardware, the executable, the run directories. Also: no debug-layer message of severity WARNING or worse.
// I-0138 tranche 4: with the profile warp-ours or hardware-ours the stage draws the plans with our own programmes
// (post_glare_programs.h) and the same comparison with the original's captured draws is made, under the same rule; then a second
// stage draws each plan with the library's programmes on the same device and every surface must equal ours byte for byte, on any
// adapter. The same comparison is then made away from the captures: the last run's camera at three other extents, with every base
// tap count the game's NumBaseSamples can ask (2 to 8), both tap parities and three blur lengths, from two synthetic scenes, a
// synthetic depth and a synthetic object velocity; every tap count of the library's table (2 to 8) must have been drawn. A plan
// with a technique that has no pinned original fails the test. On the scene of special values (infinities, NaNs) a surface may
// differ from the library's by which NaN a texel holds, and only so (counted apart: see `against`).
#include "mb_capture.h"
#include "synthetic_scene.h"
#include "post_test_programmes.h"
#include "../renderer/glare_stage.h"
#include "../renderer/post_glare_programs.h"
#include "../world/yebis_pins.h"
#include "../content/acquire.h"
#include <dxgi1_6.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <map>
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
ComPtr<ID3D12Resource> Texture(ID3D12Device* device,ID3D12GraphicsCommandList* list,std::vector<ComPtr<ID3D12Resource>>& keep,DXGI_FORMAT format,UINT w,UINT h,UINT texel,
    const uint8_t* bytes,D3D12_RESOURCE_STATES after,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE,DXGI_FORMAT copy_format=DXGI_FORMAT_UNKNOWN){
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=w;desc.Height=h;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=format;desc.Flags=flags;
    ComPtr<ID3D12Resource> t;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&t)),"texture");
    const UINT pitch=(w*texel+255)/256*256;const auto upload=Buffer(device,D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*h,D3D12_RESOURCE_STATE_GENERIC_READ);keep.push_back(upload);
    {uint8_t* memory{};const D3D12_RANGE none{};Check(upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"texture map");
        for(UINT y=0;y<h;++y)std::memcpy(memory+size_t(y)*pitch,bytes+size_t(y)*w*texel,size_t(w)*texel);upload->Unmap(0,nullptr);}
    D3D12_TEXTURE_COPY_LOCATION to{t.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    from.PlacedFootprint.Footprint={copy_format==DXGI_FORMAT_UNKNOWN?format:copy_format,w,h,1,pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    Transition(list,t.Get(),D3D12_RESOURCE_STATE_COPY_DEST,after);return t;}
}
int main(int argc,char** argv){try{
    Require(argc>=4,"usage: mb_stage_test warp|hardware[-ours] <DarkSoulsIII.exe> <Reference run directory>...");
    std::string profile=argv[1];const bool ours=profile.size()>5 && !profile.compare(profile.size()-5,5,"-ours");if(ours)profile.resize(profile.size()-5);
    const bool warp=profile=="warp";Require(warp || profile=="hardware","unknown profile");
    const auto exe=glare_capture::FileBytes(argv[2]);
    if(exe.empty()){std::printf("PASS motion blur stage %s: skipped, no executable\n",argv[1]);return 0;}
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
    size_t zones{},blurs{},blurs_exact{};uint64_t velocity_differing{},blur_changed{},blur_differing{},blur_texels{};uint32_t velocity_largest{},velocity_other{};bool same_everywhere=!warp;
    size_t against_library{},equal_library{},away_plans{},away_surfaces{},away_equal{},away_nan{},away_refused{};std::set<std::string> unpinned_names,away_techniques;
    uint64_t generation{};
    // One recording of a plan by a stage with a set of programmes, from a scene (RGBA16F), a depth's plane 0 (one float a texel)
    // and the engine's object velocity (RGBA8, OW x OH), all in tight rows. Returns each draw's surface (RGBA16F, tight rows; empty
    // when a later draw writes it again: surfaces 1 and 2 alternate).
    const auto draw_plan=[&](GlareStage& with,const GlarePrograms& programs,const yebis::GlarePlan& plan,const std::vector<uint8_t>& scene,const std::vector<uint8_t>& plane0,
            const std::vector<uint8_t>& object,UINT OW,UINT OH){
        const UINT W=plan.width,H=plan.height;Require(scene.size()==size_t(W)*H*8 && plane0.size()==size_t(W)*H*4 && object.size()==size_t(OW)*OH*4,"a recording's inputs' sizes");
        Check(allocator->Reset(),"allocator reset");ComPtr<ID3D12GraphicsCommandList> list;
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");
        std::vector<ComPtr<ID3D12Resource>> keep;
        const auto t_scene=Texture(device.Get(),list.Get(),keep,DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,8,scene.data(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        const auto t_object=Texture(device.Get(),list.Get(),keep,DXGI_FORMAT_R8G8B8A8_UNORM,OW,OH,4,object.data(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        const auto t_depth=Texture(device.Get(),list.Get(),keep,DXGI_FORMAT_R32G8X24_TYPELESS,W,H,4,plane0.data(),D3D12_RESOURCE_STATE_DEPTH_READ|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,DXGI_FORMAT_R32_TYPELESS);
        const auto capacity=post_test::TestConstantBytes(plan.draws.size());
        const auto constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,capacity,D3D12_RESOURCE_STATE_GENERIC_READ);
        uint8_t* constant_memory{};{const D3D12_RANGE none{};Check(constants->Map(0,&none,reinterpret_cast<void**>(&constant_memory)),"constants map");}
        // I-0138 tranche 8b: the library's programmes (the second stage, or the first without -ours) read the library's registers.
        post_test::TestConstants block_memory(constant_memory,constants->GetGPUVirtualAddress(),capacity,&with==&library_stage || !ours);const auto sink=block_memory.Sink();
        auto* output=with.Record(list.Get(),t_scene.Get(),++generation,plan,programs,sink,reason,nullptr,t_depth.Get(),t_object.Get());Require(output!=nullptr,reason.c_str());
        Require(!with.NeedsLayout(plan),"the stage would relay out the plan it just drew (a relayout every frame)");
        // Readbacks are taken after all draws: a surface written again later holds the later pass.
        std::map<int8_t,size_t> last_writer;for(size_t k=0;k<plan.draws.size();++k)last_writer[plan.draws[k].target]=k;
        struct Read {ComPtr<ID3D12Resource> buffer;UINT pitch{},width{},height{};size_t draw{};};std::vector<Read> reads;
        for(size_t k=0;k<plan.draws.size();++k){if(last_writer[plan.draws[k].target]!=k)continue;
            const auto& s=plan.surfaces[size_t(plan.draws[k].target)];auto* r=with.Surface(size_t(plan.draws[k].target));Require(s.half_float,"a motion blur surface that is not RGBA16F");
            const UINT pitch=(s.width*8+255)/256*256;auto rb=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(pitch)*s.height,D3D12_RESOURCE_STATE_COPY_DEST);
            Transition(list.Get(),r,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
            D3D12_TEXTURE_COPY_LOCATION dst{rb.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},src{r,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
            dst.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,s.width,s.height,1,pitch};list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            Transition(list.Get(),r,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);reads.push_back({rb,pitch,s.width,s.height,k});}
        block_memory.Spread();Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        const HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(queue->Signal(fence.Get(),++fence_value),"control signal");Check(fence->SetEventOnCompletion(fence_value,event),"control wait");
        WaitForSingleObject(event,INFINITE);CloseHandle(event);
        std::vector<std::vector<uint8_t>> rows(plan.draws.size());
        for(const auto& r:reads){const size_t row=size_t(r.width)*8;uint8_t* memory{};const D3D12_RANGE range{0,SIZE_T(r.pitch)*r.height};
            Check(r.buffer->Map(0,&range,reinterpret_cast<void**>(&memory)),"readback map");
            rows[r.draw].resize(row*r.height);for(UINT y=0;y<r.height;++y)std::memcpy(rows[r.draw].data()+size_t(y)*row,memory+size_t(y)*r.pitch,row);
            const D3D12_RANGE none{};r.buffer->Unmap(0,&none);}
        return rows;};
    // Ours against the library's, a plan through both stages from the same inputs: the surfaces compared, the equal ones counted.
    // With `which_nan` (the scene of special values alone), a surface whose only differences are one NaN for another (both hold a
    // NaN in every half that differs, of any sign and payload) is counted there instead of failing: which NaN comes out of an
    // operation on two is the adapter's choice for each compilation, and ours are compiled IEEE-strict where the originals are not
    // (WARP gives another NaN at some texels; the card that drew the captures gives the same bytes).
    const auto against=[&](const yebis::GlarePlan& plan,const GlarePrograms& mine,const GlarePrograms& theirs,const std::vector<uint8_t>& scene,const std::vector<uint8_t>& plane0,
            const std::vector<uint8_t>& object,UINT OW,UINT OH,const std::string& what,size_t& compared,size_t& equal,size_t* which_nan=nullptr){
        const auto a=draw_plan(stage,mine,plan,scene,plane0,object,OW,OH),b=draw_plan(library_stage,theirs,plan,scene,plane0,object,OW,OH);
        for(size_t k=0;k<a.size();++k){if(a[k].empty())continue;++compared;
            if(a[k]==b[k])++equal;
            else{uint64_t differing{},otherwise{};const auto nan=[](uint16_t h){return (h&0x7C00)==0x7C00 && (h&0x03FF);};
                for(size_t t=0;t<a[k].size();t+=8){if(!std::memcmp(a[k].data()+t,b[k].data()+t,8))continue;++differing;
                    for(int c=0;c<4;++c){uint16_t u{},v{};std::memcpy(&u,a[k].data()+t+2*c,2);std::memcpy(&v,b[k].data()+t+2*c,2);if(u!=v && !(nan(u) && nan(v))){++otherwise;break;}}}
                if(which_nan && !otherwise){++*which_nan;continue;}
                std::printf("%s draw %zu %s: our programme's surface differs from the library's at %llu texels, %llu of them otherwise than by one NaN for another\n",what.c_str(),k,plan.draws[k].technique.c_str(),
                    (unsigned long long)differing,(unsigned long long)otherwise);}}};
    for(int a=3;a<argc;++a){const std::string dir=argv[a];const std::string name=dir.substr(dir.find_last_of("/\\")+1);
        const auto library=glare_capture::FileBytes(dir+"/reference-shaft-alpha-library.raw");Require(library.size()>=0xD00,"a zone's library capture");
        const auto rows=glare_capture::DrawRows(dir);size_t first=rows.size();
        for(size_t n=0;n<rows.size();++n)if(glare_capture::Text(rows[n],"pass").rfind("tech_MotionBlurMakeCameraVelocity",0)==0){first=n;break;}
        Require(first<rows.size(),"a zone's captured velocity draw");
        const auto in=mb_capture::Inputs(library);const auto plan=yebis::BuildMotionBlur(yebis::MotionBlurData{},in);
        Require(plan.refused.empty(),("a zone's plan is refused: "+plan.refused).c_str());
        const UINT W=plan.width,H=plan.height,VW=in.velocity_width,VH=in.velocity_height;
        char file[96]{};std::snprintf(file,sizeof(file),"/reference-draw-%02zu-t0.raw",first);const auto depth=glare_capture::FileBytes(dir+file);
        std::snprintf(file,sizeof(file),"/reference-draw-%02zu-t1.raw",first);const auto object=glare_capture::FileBytes(dir+file);
        std::snprintf(file,sizeof(file),"/reference-draw-%02zu-t0.raw",first+1);const auto scene=glare_capture::FileBytes(dir+file);
        Require(depth.size()==size_t(W)*H*8 && object.size()==size_t(VW)*VH*4 && scene.size()==size_t(W)*H*8,"a zone's captured inputs' sizes");
        std::vector<uint8_t> plane0(size_t(W)*H*4);for(size_t i=0;i<size_t(W)*H;++i)std::memcpy(plane0.data()+i*4,depth.data()+i*8,4);
        std::string captured_adapter;{std::snprintf(file,sizeof(file),"/reference-draw-%02zu-adapter.txt",first+1);std::ifstream f(dir+file);std::getline(f,captured_adapter);}
        same_everywhere=same_everywhere && captured_adapter==this_adapter;
        GlarePrograms library_programs;{std::string unpinned;Require(LibraryProgrammes(exe,plan,library_programs,unpinned),"a motion blur technique without a pin");}
        const auto drawn=draw_plan(stage,ours?OurProgrammes(plan):library_programs,plan,scene,plane0,object,VW,VH);
        for(size_t k=0;k<drawn.size();++k){if(drawn[k].empty())continue;const auto& s=plan.surfaces[size_t(plan.draws[k].target)];const size_t draw=first+k;
            std::snprintf(file,sizeof(file),"/reference-draw-%02zu-output.raw",draw);const auto captured=glare_capture::FileBytes(dir+file);
            Require(captured.size()==drawn[k].size(),"a captured output's size");
            // Review: per channel, the largest distance in half-float units (positive halves order as integers; the sign folded).
            uint64_t differing{};std::array<uint32_t,4> largest{};
            for(size_t t=0;t<captured.size();t+=8){const uint8_t* mine=drawn[k].data()+t;const uint8_t* theirs=captured.data()+t;
                if(!std::memcmp(mine,theirs,8))continue;++differing;
                for(int c=0;c<4;++c){uint16_t u{},v{};std::memcpy(&u,mine+2*c,2);std::memcpy(&v,theirs+2*c,2);const auto key=[](uint16_t h){return (h&0x8000)?-int(h&0x7FFF):int(h);};
                    largest[size_t(c)]=std::max(largest[size_t(c)],uint32_t(std::abs(key(u)-key(v))));}}
            // Review: the texels the original's pass changed (its output against its input t0): a pass that changes nothing would let the
            // comparison pass whatever the velocity holds.
            if(k>0){std::snprintf(file,sizeof(file),"/reference-draw-%02zu-t0.raw",draw);const auto input=glare_capture::FileBytes(dir+file);Require(input.size()==captured.size(),"a blur input's size");
                uint64_t changed{};for(size_t i=0;i<captured.size();i+=8)changed+=std::memcmp(input.data()+i,captured.data()+i,8)!=0;
                std::printf("%s draw %zu: the original's pass changes %llu texels\n",name.c_str(),draw,(unsigned long long)changed);blur_changed+=changed;}
            else{std::printf("%s velocity: largest per channel %u %u %u %u\n",name.c_str(),largest[0],largest[1],largest[2],largest[3]);velocity_largest=std::max({velocity_largest,largest[0],largest[1]});velocity_other=std::max({velocity_other,largest[2],largest[3]});}
            std::printf("%s draw %zu %s %ux%u: %llu of %u texels differ\n",name.c_str(),draw,plan.draws[k].technique.c_str(),s.width,s.height,(unsigned long long)differing,s.width*s.height);
            if(k==0)velocity_differing+=differing;else{++blurs;if(!differing)++blurs_exact;blur_differing+=differing;blur_texels+=uint64_t(s.width)*s.height;}}
        ++zones;
        if(!ours)continue;
        // I-0138 tranche 4: our programmes against the library's on this device, the capture's inputs.
        against(plan,OurProgrammes(plan),library_programs,scene,plane0,object,VW,VH,name,against_library,equal_library);
        if(a!=argc-1)continue;   // the sweep below once, with the last run's camera (the build names the turning camera last: its two poses differ)
        // Away from the captures: the zone's camera at other extents (a small window, an odd size, 16:10; the aspect the extent's,
        // the internal velocity's and the object velocity's extents as the frame sets them), every base tap count, both parities,
        // three blur lengths; two synthetic scenes, a synthetic depth and object velocity. A plan the builder refuses is counted; a
        // plan seen before (the same extent, passes and techniques with the same step) is not drawn again.
        static constexpr UINT extents[3][2]{{800,450},{1501,843},{1680,1050}};static constexpr float lengths[3]{0.02f,0.06f,0.2f};
        for(const auto& extent:extents){const UINT EW=extent[0],EH=extent[1],OW=EW>>1,OH=EH>>1;std::set<std::string> seen;
            const auto synthetic_depth=synthetic::SyntheticDepth(EW,EH),synthetic_object=synthetic::SyntheticVelocity(OW,OH);
            const std::array<std::vector<uint8_t>,2> scenes{synthetic::SyntheticScene(EW,EH,false),synthetic::SyntheticScene(EW,EH,true)};
            for(int base=2;base<=8;++base)for(const bool bidirectional:{true,false})for(const float length:lengths){auto sized=in;
                sized.width=EW;sized.height=EH;sized.camera.aspect=float(EW)/float(EH);
                sized.velocity_width=uint32_t(int64_t(float(EW)*0.5f+0.99f));sized.velocity_height=uint32_t(int64_t(float(sized.velocity_width)/sized.camera.aspect+0.499999f));
                sized.base_samples=base;sized.bidirectional=bidirectional;sized.max_length=length;
                const auto other=yebis::BuildMotionBlur(yebis::MotionBlurData{},sized);
                if(!other.refused.empty()){++away_refused;continue;}
                char key[160]{};std::snprintf(key,sizeof(key),"%d/%d/%d/%.9g",other.base_taps,other.passes,other.taps,other.step);if(!seen.insert(key).second)continue;
                GlarePrograms theirs;std::string unpinned;
                if(!LibraryProgrammes(exe,other,theirs,unpinned)){std::printf("%s at %ux%u: %s has no pinned original: not compared\n",name.c_str(),EW,EH,unpinned.c_str());unpinned_names.insert(unpinned);continue;}
                const auto mine=OurProgrammes(other);for(const auto& g:other.draws)away_techniques.insert(g.technique);
                char what[200]{};std::snprintf(what,sizeof(what),"%s at %ux%u (base %d, %d passes, last %d)",name.c_str(),EW,EH,other.base_taps,other.passes,other.taps);
                for(size_t s=0;s<scenes.size();++s)against(other,mine,theirs,scenes[s],synthetic_depth,synthetic_object,OW,OH,std::string(what)+(s?" (special values)":""),away_surfaces,away_equal,s?&away_nan:nullptr);
                ++away_plans;}}}
    ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");uint64_t messages{};
    for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::fprintf(stderr,"%s\n",message->pDescription);}}
    Require(messages==0,"debug-layer messages");
    if(!zones){std::printf("PASS motion blur stage %s: skipped, no zone capture\n",argv[1]);return 0;}
    // Review: on every adapter the velocity surface may differ from the original's only in its velocity channels (red and
    // green: MB-0, red at m37 and m31, green at m50) and by at most one half unit on the capture's adapter; elsewhere the units are reported.
    Require(velocity_other==0 || !same_everywhere,"the velocity surface differs outside its red and green channels on the capture's adapter");
    if(same_everywhere)Require(velocity_largest<=1,"the velocity surface's red or green differs by more than one half unit on the capture's adapter");
    // The blur passes are exact on the original's own inputs (T13 replays draws 12-13); chained after our velocity surface they inherit its
    // half-unit texels: with a moving camera (Reference1693) 47 and 228 of 921600. Required: the original's passes change texels at one run at
    // least (a moving camera), and on the capture's adapter the chain differs on at most 0.1% of the blurred texels.
    Require(blur_changed>0,"no run where the original's blur changes a texel (a still camera everywhere: the comparison would be vacuous)");
    if(same_everywhere)Require(blur_differing*1000<=blur_texels,"the native motion blur's passes differ from the original on more than 0.1% of the texels on the capture's adapter");
    if(ours){std::printf("our programmes against the library's on this device: %zu of %zu surfaces byte for byte\n",equal_library,against_library);
        Require(against_library && equal_library==against_library,"our motion blur programmes differ from the library's");
        std::string names,drawn_names;for(const auto& technique:unpinned_names)names+=(names.empty()?"":", ")+technique;
        for(const auto& technique:away_techniques)drawn_names+=(drawn_names.empty()?"":", ")+technique.substr(technique.find_last_of('_')+1);
        std::printf("our programmes against the library's away from the captures (three extents, the base tap counts 2 to 8, both parities, three lengths; two synthetic scenes, a synthetic depth "
            "and object velocity): %zu of %zu surfaces byte for byte in %zu plans, %zu more equal but for which NaN (the special values' scene only); %zu plans refused by the builder; techniques drawn: %s; "
            "without a pinned original: %s\n",away_equal,away_surfaces,away_plans,away_nan,away_refused,drawn_names.c_str(),names.empty()?"none":names.c_str());
        Require(away_surfaces && away_equal+away_nan==away_surfaces,"our motion blur programmes differ from the library's away from the captures");
        // The final review's L4: the which-NaN surfaces are WARP's; on the adapter and driver that drew the captures the bytes are equal.
        if(same_everywhere)Require(away_nan==0,"a surface differs from the library's by which NaN on the capture's adapter (equal bytes there until now)");
        for(int n=2;n<=8;++n)Require(away_techniques.contains("tech_MotionBlurVelocityDirDepthInnerscreen_SM3_T2_S"+std::to_string(n)),"a tap count of the library's table was not drawn away from the captures");
        Require(unpinned_names.empty(),"a plan names a technique without a pinned original (pin it from the library's technique table: yebis_pins.h)");}
    std::printf("PASS motion blur stage %s: T16: the native plans drawn by the glare stage from the original's depth, object velocity and scene: %zu of %zu blur outputs at %zu zones "
        "equal the original's, %llu of %llu texels differ%s; the original's passes change %llu texels; the velocity surfaces differ at %llu texels (red and green by at most %u, blue and alpha by at most %u half units); "
        "no debug-layer message\n",argv[1],blurs_exact,blurs,zones,(unsigned long long)blur_differing,(unsigned long long)blur_texels,
        same_everywhere?" (the capture's adapter and driver: at most 0.1%)":" (another adapter, driver or WARP: reported)",
        (unsigned long long)blur_changed,(unsigned long long)velocity_differing,velocity_largest,velocity_other);
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
