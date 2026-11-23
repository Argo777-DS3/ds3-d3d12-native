// W10 GR T17 (W10-NATIVE-PASSES 12): our colour grading pass against the original's own draw. Reference1696 (m31, frame 1200,
// yebis-draws-capture.flag) captured the original's tech_MadColorGradingLutT1_S1 draw (draw 50; the test finds a capture's grading draw by its row): its input t0 (the tone map's
// output, R8G8B8A8 1280x720), its LUT t16 (R32G32B32A32_FLOAT 16x16x16, x fastest, then y, then z), its output and its `$Globals`
// block (the row reference_yebis_draw). The 16x256 strip is rebuilt from t16 (every captured word is k/255, the UNORM texel of the
// map's m31_00_CGrading0001: 3D[x][y][z] = 2D[x][16z + y], checked against the TPF by Compare-GradingLut.ps1: evidence grading-lut-vs-tpf.txt, 0 of 4096), then the stage copies it into its volume
// with GX's LutCopy2DTo3DCs (programme cache) and draws t0 through it with the original's programmes (the executable, by their pins).
// Checks: the words the programmes read equal the captured block's; the volume equals the captured t16 bit for bit; the output equals
// the original's (exactly on the capture's adapter and driver, within 1/255 elsewhere); no debug-layer message. Arguments:
// warp|hardware, the executable, the programme cache directory, the capture directory (skipped with a message without them).
#include "../renderer/grading_stage.h"
#include "../renderer/yebis_stage.h"
#include "../world/yebis_pins.h"
#include "../content/acquire.h"
#include <dxgi1_6.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
std::vector<uint8_t> FileBytes(const std::string& path){std::ifstream file(path,std::ios::binary);if(!file)return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;}
ComPtr<ID3D12Resource> Texture(ID3D12Device* device,DXGI_FORMAT format,UINT w,UINT h,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state){
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width=w;desc.Height=h;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=format;desc.Flags=flags;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control texture");return result;}
// The technique's two pinned containers, found in the executable file.
bool Programmes(const std::vector<uint8_t>& exe,GradingBlobs& blobs){
    const auto* pin=yebis::FindTechniquePin("tech_MadColorGradingLutT1_S1");if(!pin)return false;
    for(size_t at=0;at+32<=exe.size();at+=4){
        if(std::memcmp(exe.data()+at,"DXBC",4))continue;uint32_t size{};std::memcpy(&size,exe.data()+at+24,4);
        if((size!=2624 && size!=2876) || at+size>exe.size())continue;char hex[65]{};if(!ds3content::Sha256(exe.data()+at,size,hex))continue;
        if(blobs.vertex.empty() && !std::strcmp(hex,pin->vs_sha256))blobs.vertex.assign(exe.begin()+at,exe.begin()+at+size);
        if(blobs.pixel.empty() && !std::strcmp(hex,pin->ps_sha256))blobs.pixel.assign(exe.begin()+at,exe.begin()+at+size);}
    return !blobs.vertex.empty() && !blobs.pixel.empty();}
// The captured grading draw (W10 closure: any zone): the first row reference_yebis_draw of tech_MadColorGradingLutT1_S1 whose draw the
// capture dumped (reference-draw-NN-t16.raw present); its prefix reference-draw-NN- and its `$Globals` block.
struct CapturedDraw {std::string prefix;std::vector<uint8_t> globals;};
// Review: the row of the captured frame (the frame of the reference_yebis_draw rows just before the first reference_yebis_draw_capture
// row), so the block and the pixels come from the same frame.
CapturedDraw FindGradingDraw(const std::string& dir){
    std::string frame;
    {std::ifstream trace(dir+"/startup.jsonl");std::string line,last;
        while(std::getline(trace,line)){
            if(line.find("\"event\":\"reference_yebis_draw\",\"frame\":")!=std::string::npos){const auto at=line.find("\"frame\":")+8;last=line.substr(at,line.find(',',at)-at);}
            else if(line.find("\"event\":\"reference_yebis_draw_capture\"")!=std::string::npos){frame=last;break;}}}
    if(frame.empty())return {};
    std::ifstream trace(dir+"/startup.jsonl");std::string line;
    while(std::getline(trace,line)){
        if(line.find("\"event\":\"reference_yebis_draw\",\"frame\":"+frame+",")==std::string::npos || line.find("\"pass\":\"tech_MadColorGradingLutT1_S1\"")==std::string::npos)continue;
        const auto d=line.find("\"draw\":");if(d==std::string::npos)continue;
        char prefix[32]{};std::snprintf(prefix,sizeof(prefix),"/reference-draw-%02lu-",std::stoul(line.substr(d+7)));
        if(!std::ifstream(dir+prefix+"t16.raw"))continue;
        const auto at=line.find("\"globals\":\"");if(at==std::string::npos)return {};
        CapturedDraw out{prefix,{}};for(size_t i=at+11;i+1<line.size() && line[i]!='"';i+=2)out.globals.push_back(uint8_t(std::stoul(line.substr(i,2),nullptr,16)));return out;}
    return {};}
}
int main(int argc,char** argv){try{
    Require(argc==5,"usage: grading_stage_test warp|hardware <DarkSoulsIII.exe> <programme cache> <capture directory>");const bool warp=!std::strcmp(argv[1],"warp");
    Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
    const std::string dir=argv[4];
    const auto exe=FileBytes(argv[2]);const auto copy=FileBytes(std::string(argv[3])+"/gxposteffect__LutCopy2DTo3DCs.cpo");
    const auto draw=FindGradingDraw(dir);const std::string p=dir+(draw.prefix.empty()?std::string("/reference-draw-none-"):draw.prefix);
    const auto t0=FileBytes(p+"t0.raw"),t16=FileBytes(p+"t16.raw"),original=FileBytes(p+"output.raw");
    Require(!std::ifstream(dir+"/startup.jsonl") || !draw.prefix.empty(),"the capture has a trace but no grading draw at its captured frame");   // review: no silent skip
    if(exe.empty() || copy.empty() || t0.empty()){std::printf("PASS grading stage %s: skipped, no executable, programme cache or capture\n",argv[1]);return 0;}
    constexpr UINT W=1280,H=720,N=GradingStage::LutEdge;
    Require(t0.size()==size_t(W)*H*4 && original.size()==t0.size() && t16.size()==size_t(N)*N*N*16,"capture sizes");
    GradingBlobs blobs;Require(Programmes(exe,blobs),"the pinned programmes are not in the executable");blobs.lut_copy=copy;
    {const auto* pin=&yebis_gx_pins[1];char hex[65]{};Require(!std::wcscmp(pin->name,L"LutCopy2DTo3DCs"),"pin table");
        Require(pin && copy.size()==pin->bytes && ds3content::Sha256(copy.data(),copy.size(),hex) && !std::strcmp(hex,pin->sha256),"LutCopy2DTo3DCs differs from its pin");}
    // The words the programmes read, against the captured block.
    {std::array<uint8_t,GradingStage::GlobalsBytes> ours{};GradingStage::Globals(ours.data());const auto& captured=draw.globals;Require(captured.size()>=3008,"the captured block");
        for(const auto& [at,bytes]:std::array<std::pair<size_t,size_t>,4>{{{80,8},{544,16},{2464,16},{2976,32}}})
            Require(!std::memcmp(ours.data()+at,captured.data()+at,bytes),"the grading block differs from the captured words");}
    // The strip from the volume: 2D[x][16z + y] = round(255 x 3D[x][y][z]), each word exactly k/255.
    std::vector<uint8_t> strip(size_t(N)*N*N*4);
    for(UINT z=0;z<N;++z)for(UINT y=0;y<N;++y)for(UINT x=0;x<N;++x)for(UINT c=0;c<4;++c){float v{};std::memcpy(&v,t16.data()+(((size_t(z)*N+y)*N+x)*4+c)*4,4);
        const long k=std::lround(v*255.0f);Require(k>=0 && k<=255 && float(double(k)/255.0)==v,"a LUT word is not an 8-bit UNORM value");
        strip[((size_t(z)*N+y)*N+x)*4+c]=uint8_t(k);}
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"control factory");ComPtr<IDXGIAdapter1> adapter;
    if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"control WARP adapter");
    else{Check(factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter)),"control hardware adapter");
        DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);Require(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE),"no hardware adapter");}
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"control device");
    const D3D12_COMMAND_QUEUE_DESC queue_desc{D3D12_COMMAND_LIST_TYPE_DIRECT};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)),"control queue");
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"control allocator");
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"control fence");const HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    GradingStage stage;std::string reason;Require(stage.Open(device.Get(),blobs,DXGI_FORMAT_R8G8B8A8_UNORM,W,H,reason),reason.c_str());
    // The tone-mapped image into the stage's source (a copy: the stage's source is a render target), the strip, the target.
    const auto lut=Texture(device.Get(),DXGI_FORMAT_R8G8B8A8_UNORM,N,N*N,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto target=Texture(device.Get(),DXGI_FORMAT_R8G8B8A8_UNORM,W,H,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET);
    const UINT pitch=W*4,strip_pitch=256;   // 16 texels x 4 bytes = 64, aligned to 256
    const auto image_upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(pitch)*H,D3D12_RESOURCE_STATE_GENERIC_READ);
    const auto strip_upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(strip_pitch)*N*N,D3D12_RESOURCE_STATE_GENERIC_READ);
    const auto readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(pitch)*H,D3D12_RESOURCE_STATE_COPY_DEST);
    const UINT volume_pitch=256;const auto volume_readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(volume_pitch)*N*N,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,GradingStage::GlobalsBytes,D3D12_RESOURCE_STATE_GENERIC_READ);
    {uint8_t* memory{};const D3D12_RANGE none{};Check(image_upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"image map");std::memcpy(memory,t0.data(),t0.size());image_upload->Unmap(0,nullptr);
        Check(strip_upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"strip map");
        for(UINT row=0;row<N*N;++row)std::memcpy(memory+size_t(row)*strip_pitch,strip.data()+size_t(row)*N*4,N*4);strip_upload->Unmap(0,nullptr);}
    void* constant_memory{};{const D3D12_RANGE none{};Check(constants->Map(0,&none,&constant_memory),"constants map");}
    D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};ComPtr<ID3D12DescriptorHeap> rtv;Check(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv)),"control rtv heap");
    device->CreateRenderTargetView(target.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    {auto barrier=Transition(stage.SourceResource(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);list->ResourceBarrier(1,&barrier);}
    D3D12_TEXTURE_COPY_LOCATION to{stage.SourceResource(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{image_upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    from.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,W,H,1,pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    to.pResource=lut.Get();from.pResource=strip_upload.Get();from.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,N,N*N,1,strip_pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    D3D12_RESOURCE_BARRIER barriers[2]{Transition(stage.SourceResource(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET),
        Transition(lut.Get(),D3D12_RESOURCE_STATE_COPY_DEST,static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))};
    list->ResourceBarrier(2,barriers);
    stage.Record(list.Get(),lut.Get(),DXGI_FORMAT_R8G8B8A8_UNORM,1,rtv->GetCPUDescriptorHandleForHeapStart(),[&](uint64_t bytes){
        Require(bytes==GradingStage::GlobalsBytes,"constant request");return GradingConstantMemory{constant_memory,constants->GetGPUVirtualAddress()};});
    D3D12_RESOURCE_BARRIER after[2]{Transition(target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE),
        Transition(stage.Volume(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE)};list->ResourceBarrier(2,after);
    D3D12_TEXTURE_COPY_LOCATION out{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},image{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    out.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,W,H,1,pitch};list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
    out.pResource=volume_readback.Get();out.PlacedFootprint.Footprint={DXGI_FORMAT_R32G32B32A32_FLOAT,N,N,N,volume_pitch};image.pResource=stage.Volume();
    list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
    {auto barrier=Transition(stage.Volume(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);}
    Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
    Check(queue->Signal(fence.Get(),1),"control signal");Check(fence->SetEventOnCompletion(1,event),"control wait");WaitForSingleObject(event,INFINITE);CloseHandle(event);
    std::vector<uint8_t> pixels(size_t(W)*H*4),volume(t16.size());
    {uint8_t* memory{};const D3D12_RANGE read{0,SIZE_T(pitch)*H};Check(readback->Map(0,&read,reinterpret_cast<void**>(&memory)),"control readback");
        std::memcpy(pixels.data(),memory,pixels.size());const D3D12_RANGE none{};readback->Unmap(0,&none);
        const D3D12_RANGE read_volume{0,SIZE_T(volume_pitch)*N*N};Check(volume_readback->Map(0,&read_volume,reinterpret_cast<void**>(&memory)),"control volume readback");
        for(UINT row=0;row<N*N;++row)std::memcpy(volume.data()+size_t(row)*N*16,memory+size_t(row)*volume_pitch,N*16);volume_readback->Unmap(0,&none);}
    Require(volume==t16,"the volume differs from the original's LUT");
    std::array<uint64_t,4> differing{};int largest{};
    for(size_t i=0;i<pixels.size();++i){const int diff=std::abs(int(pixels[i])-int(original[i]));if(!diff)continue;++differing[i%4];largest=std::max(largest,diff);}
    std::printf("colour grading against the original: differing bytes r %llu g %llu b %llu a %llu of %u each; largest %d/255\n",(unsigned long long)differing[0],
        (unsigned long long)differing[1],(unsigned long long)differing[2],(unsigned long long)differing[3],W*H,largest);
    ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");uint64_t messages{};
    for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::fprintf(stderr,"%s\n",message->pDescription);}}
    Require(messages==0,"debug-layer messages");
    // Exactness on the adapter and driver that drew the capture (reference-draw-NN-adapter.txt); elsewhere the trilinear filtering
    // of the volume may round differently: within 1/255 and on fewer than 5% of the bytes.
    std::string captured_adapter;{std::ifstream file(p+"adapter.txt");std::getline(file,captured_adapter);}
    std::string this_adapter;{DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);LARGE_INTEGER driver{};(void)adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice),&driver);
        char text[96]{};std::snprintf(text,sizeof(text),"%04X %04X %016llX",desc.VendorId,desc.DeviceId,static_cast<unsigned long long>(driver.QuadPart));this_adapter=text;}
    const bool same=!warp && !captured_adapter.empty() && captured_adapter==this_adapter;
    const uint64_t total=differing[0]+differing[1]+differing[2]+differing[3];
    if(same)Require(largest==0,"the native grading differs from the original on the capture's adapter");
    else Require(largest<=1 && total*20<uint64_t(W)*H*4,"the native grading differs from the original beyond its bound");
    std::printf("PASS grading stage %s: T17 %s: the block's words, the LUT copy (the volume bit for bit) and the grading draw from the original's input equal "
        "its output within %d/255 (%s), no debug-layer message\n",argv[1],dir.substr(dir.find_last_of("/\\")+1).c_str(),largest,
        same?"the capture's adapter and driver: exactness required":"another adapter, driver or WARP: within 1/255");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
