// W10 TM-2 T12 (W10-NATIVE-PASSES 3.2): our tone-map pass against the original's own draw. The Reference runs
// i66-reference-1638 and 1643 (m37) captured, after the original's TonemapHDR_Dither_Exposure_Matrix_Gamma draw at m37
// (frame 1200), its scene input t0 (R16G16B16A16 1280x720), its glare input t1 (R11G11B10 420x354) and its output (R8G8B8A8).
// The stage draws the same inputs with the programmes found in the executable by their pins (yebis_pins.h), the `$Globals` block
// built natively (yebis::BuildTonemap, TonemapGlobals: the m37 Tone Map record and the executable's data words) and the dither
// texels of 141719770; the output is compared with the original's, byte for byte. Arguments: warp|hardware, the executable, the
// capture directory (skipped with a message without either). Also: no debug-layer message of severity WARNING or worse.
#include "../renderer/tonemap_stage.h"
#include "../world/yebis_tonemap.h"
#include "../world/yebis_pins.h"
#include "../world/light_producers.h"
#include "../content/acquire.h"
#include <dxgi1_6.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
// The two pinned containers of the technique, found in the executable file.
bool Programmes(const std::vector<uint8_t>& exe,TonemapBlobs& blobs){
    const auto* pin=yebis::FindTechniquePin("tech_TonemapHDR_Dither_Exposure_Matrix_Gamma");if(!pin)return false;
    const auto* vignette=yebis::FindTechniquePin("tech_TonemapHDR_Dither_Vignette_Exposure_Matrix_Gamma");if(!vignette)return false;
    for(size_t at=0;at+32<=exe.size();at+=4){
        if(std::memcmp(exe.data()+at,"DXBC",4))continue;uint32_t size{};std::memcpy(&size,exe.data()+at+24,4);
        if((size!=2980 && size!=4164 && size!=4356) || at+size>exe.size())continue;char hex[65]{};if(!ds3content::Sha256(exe.data()+at,size,hex))continue;
        if(!std::strcmp(hex,pin->vs_sha256))blobs.vertex.assign(exe.begin()+at,exe.begin()+at+size);
        if(!std::strcmp(hex,pin->ps_sha256))blobs.pixel.assign(exe.begin()+at,exe.begin()+at+size);
        if(!std::strcmp(hex,vignette->ps_sha256))blobs.pixel_vignette.assign(exe.begin()+at,exe.begin()+at+size);}
    return !blobs.vertex.empty() && !blobs.pixel.empty() && !blobs.pixel_vignette.empty();}
}
int main(int argc,char** argv){try{
    Require(argc==4 || argc==5,"usage: tonemap_stage_test warp|hardware <DarkSoulsIII.exe> <capture directory> [output file]");const bool warp=!std::strcmp(argv[1],"warp");
    Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
    const std::string dir=argv[3];
    const auto scene=FileBytes(dir+"/reference-tonemap-t0.raw"),glare=FileBytes(dir+"/reference-tonemap-t1.raw"),original=FileBytes(dir+"/reference-tonemap-output.raw");
    const auto exe=FileBytes(argv[2]);
    if(exe.empty() || scene.empty()){std::printf("PASS tone-map stage %s: skipped, no executable or capture\n",argv[1]);return 0;}
    constexpr UINT W=1280,H=720;const bool large_glare=glare.size()==size_t(640)*540*4;const UINT GW=large_glare?640:420,GH=large_glare?540:354;   // m50: 640x540
    Require(scene.size()==size_t(W)*H*8 && glare.size()==size_t(GW)*GH*4 && original.size()==size_t(W)*H*4,"capture sizes");
    TonemapBlobs blobs;Require(Programmes(exe,blobs),"the pinned programmes are not in the executable");
    _putenv_s("DS3_EXE",argv[2]);
    // The block from the m37 Tone Map record (Exposure 1.52, Gamma 2.2, FilmicContrast 0.3, ColorH 0, ColorS 0.9, ColorV 1).
    yebis::TonemapData d;std::array<float,16> rows{};float scale{},bias{},to_byte{};
    Require(ds3light::Rdata(0x2B60C40,d.curve.data(),84) && ds3light::Rdata(0x2B60CD0,&d.tx_scale,4) && ds3light::Rdata(0x3D7EDA4,&d.third,4) &&
        ds3light::Rdata(0x3D7EE10,&d.one,4) && ds3light::Rdata(0x3D7EF70,&d.steps,4) && ds3light::Rdata(0x2B60D60,rows.data(),16) &&
        ds3light::Rdata(0x2B60D40,rows.data()+4,16) && ds3light::Rdata(0x2B60D50,rows.data()+8,16) && ds3light::Rdata(0x2B60D30,rows.data()+12,16) &&
        ds3light::Rdata(0x3D7ED5C,&scale,4) && ds3light::Rdata(0x2A5F3CC,&bias,4) && ds3light::Rdata(0x2B60CE4,&to_byte,4),"executable data words");
    Require(yebis::LoadColorData(d.color,[](uint32_t rva,void* out,size_t bytes){return ds3light::Rdata(rva,out,bytes);}),"colour data words");   // GL-4: E69A0's matrix path
    yebis::TonemapInputs in;std::memcpy(&in.exposure,"\x5C\x8F\xC2\x3F",4);std::memcpy(&in.gamma,"\xCD\xCC\x0C\x40",4);
    std::memcpy(&in.filmic_contrast,"\x9A\x99\x99\x3E",4);in.color_h=0.0f;std::memcpy(&in.color_s,"\x66\x66\x66\x3F",4);in.color_v=1.0f;
    // A capture with reference-tonemap-tone.raw (the library's globals at the draw: exposure, gamma, tone type, filmic contrast, hue,
    // saturation, value) supplies its own record values.
    float vignette_power{};   // TM-3: words 7, 8 of a 36-byte tone file (Vignette Power, VignetteFovDependence)
    if(const auto tone=FileBytes(dir+"/reference-tonemap-tone.raw");tone.size()==28 || tone.size()==36){if(tone.size()==36)std::memcpy(&vignette_power,tone.data()+28,4);std::memcpy(&in.exposure,tone.data(),4);std::memcpy(&in.gamma,tone.data()+4,4);
        std::memcpy(&in.filmic_contrast,tone.data()+12,4);std::memcpy(&in.color_h,tone.data()+16,4);std::memcpy(&in.color_s,tone.data()+20,4);std::memcpy(&in.color_v,tone.data()+24,4);}
    std::array<uint8_t,TonemapStage::GlobalsBytes> globals{};yebis::TonemapGlobals(yebis::BuildTonemap(d,in),globals.data());
    const auto dither=yebis::TonemapDither(rows,scale,bias,to_byte);
    // TM-3: the vignette variant when the capture's Vignette Power is positive: its TEXCOORD2 computed natively (tan(pi/8) from
    // [142A66FD0], the 4/3 and 2/3 of CPostEffect::Initialize, the aspect 1280/720) must equal the captured vertices' bit for bit.
    std::array<std::array<float,2>,3> vignette_uv{};const bool vignette=0.0f<vignette_power;
    if(vignette){float eighth_pi{};uint32_t eighth_pi_bits{};Require(ds3light::Rdata(0x2A66FD0,&eighth_pi,4) && (std::memcpy(&eighth_pi_bits,&eighth_pi,4),eighth_pi_bits==0x3EC90FDBu),"pi/8 data word");
        yebis::VignetteData vd;const uint32_t tan_bits=0x3ED413CDu;std::memcpy(&vd.tan_eighth_pi,&tan_bits,4);   /* the game's constant bits */const uint32_t k_bits=0x3FAAAAA8u,m_bits=0x3F2AAAE3u;std::memcpy(&vd.k,&k_bits,4);std::memcpy(&vd.m,&m_bits,4);
        vignette_uv=yebis::VignetteCoordinates(yebis::VignetteRect(vd,vignette_power,float(W)/float(H)));
        const auto vertices=FileBytes(dir+"/reference-tonemap-vertices.raw");Require(vertices.size()>=3*48,"the capture's vertices");
        const auto indices=FileBytes(dir+"/reference-tonemap-indices.raw");   // the draw is indices 0, 1, 2 (R16) of the dumped vertices, stride 48
        Require(indices.size()>=6 && indices[0]==0 && indices[1]==0 && indices[2]==1 && indices[3]==0 && indices[4]==2 && indices[5]==0,"the capture's indices");
        for(size_t i=0;i<3;++i)for(size_t c=0;c<2;++c){float captured{};std::memcpy(&captured,vertices.data()+i*48+24+c*4,4);
            Require(!std::memcmp(&captured,&vignette_uv[i][c],4),"the vignette coordinates differ from the captured vertices");}}
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
    TonemapStage stage;std::string reason;Require(stage.Open(device.Get(),blobs,dither,DXGI_FORMAT_R8G8B8A8_UNORM,W,H,reason),reason.c_str());
    // Inputs from upload rows (pitch aligned to 256), the target, its readback.
    const auto t0=Texture(device.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto t1=Texture(device.Get(),DXGI_FORMAT_R11G11B10_FLOAT,GW,GH,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto target=Texture(device.Get(),DXGI_FORMAT_R8G8B8A8_UNORM,W,H,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET);
    const UINT scene_pitch=W*8,glare_pitch=(GW*4+255)/256*256,target_pitch=W*4;
    const auto scene_upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(scene_pitch)*H,D3D12_RESOURCE_STATE_GENERIC_READ);
    const auto glare_upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,UINT64(glare_pitch)*GH,D3D12_RESOURCE_STATE_GENERIC_READ);
    const auto readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,UINT64(target_pitch)*H,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,TonemapStage::GlobalsBytes+256,D3D12_RESOURCE_STATE_GENERIC_READ);   // the block, then 256 bytes of vignette vertices
    {uint8_t* memory{};const D3D12_RANGE none{};Check(scene_upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"scene map");std::memcpy(memory,scene.data(),scene.size());scene_upload->Unmap(0,nullptr);
        Check(glare_upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"glare map");
        for(UINT y=0;y<GH;++y)std::memcpy(memory+size_t(y)*glare_pitch,glare.data()+size_t(y)*GW*4,GW*4);glare_upload->Unmap(0,nullptr);}
    void* constant_memory{};{const D3D12_RANGE none{};Check(constants->Map(0,&none,&constant_memory),"constants map");}
    D3D12_DESCRIPTOR_HEAP_DESC heap{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};ComPtr<ID3D12DescriptorHeap> rtv;Check(device->CreateDescriptorHeap(&heap,IID_PPV_ARGS(&rtv)),"control rtv heap");
    device->CreateRenderTargetView(target.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    D3D12_TEXTURE_COPY_LOCATION to{t0.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{scene_upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
    from.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,1,scene_pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    to.pResource=t1.Get();from.pResource=glare_upload.Get();from.PlacedFootprint.Footprint={DXGI_FORMAT_R11G11B10_FLOAT,GW,GH,1,glare_pitch};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
    D3D12_RESOURCE_BARRIER barriers[2]{Transition(t0.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE),
        Transition(t1.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)};list->ResourceBarrier(2,barriers);
    stage.Record(list.Get(),t0.Get(),1,rtv->GetCPUDescriptorHandleForHeapStart(),globals.data(),[&](uint64_t bytes){
        if(bytes==256)return TonemapConstantMemory{static_cast<uint8_t*>(constant_memory)+TonemapStage::GlobalsBytes,constants->GetGPUVirtualAddress()+TonemapStage::GlobalsBytes};
        Require(bytes==TonemapStage::GlobalsBytes,"constant request");return TonemapConstantMemory{constant_memory,constants->GetGPUVirtualAddress()};},t1.Get(),
        vignette?&vignette_uv:nullptr);
    auto barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);list->ResourceBarrier(1,&barrier);
    D3D12_TEXTURE_COPY_LOCATION out{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},image{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    out.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,W,H,1,target_pitch};list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
    Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
    Check(queue->Signal(fence.Get(),1),"control signal");Check(fence->SetEventOnCompletion(1,event),"control wait");WaitForSingleObject(event,INFINITE);CloseHandle(event);
    std::vector<uint8_t> pixels(size_t(W)*H*4);{uint8_t* memory{};const D3D12_RANGE read{0,SIZE_T(target_pitch)*H};Check(readback->Map(0,&read,reinterpret_cast<void**>(&memory)),"control readback");
        std::memcpy(pixels.data(),memory,pixels.size());const D3D12_RANGE none{};readback->Unmap(0,&none);}
    // The comparison: per channel, the largest difference and the count of differing bytes (colour and alpha apart).
    std::array<uint64_t,4> differing{};int largest{};std::array<uint64_t,3> histogram{};
    for(size_t i=0;i<pixels.size();++i){const int diff=std::abs(int(pixels[i])-int(original[i]));if(!diff)continue;++differing[i%4];
        largest=std::max(largest,diff);++histogram[std::min(diff,3)-1];}
    std::printf("tone map against the original: differing bytes r %llu g %llu b %llu a %llu of %u each; largest %d/255; 1: %llu, 2: %llu, 3+: %llu\n",
        (unsigned long long)differing[0],(unsigned long long)differing[1],(unsigned long long)differing[2],(unsigned long long)differing[3],W*H,largest,
        (unsigned long long)histogram[0],(unsigned long long)histogram[1],(unsigned long long)histogram[2]);
    if(argc==5){std::ofstream file(argv[4],std::ios::binary);file.write(reinterpret_cast<const char*>(pixels.data()),std::streamsize(pixels.size()));}   // RGBA8 rows, for inspection
    ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");uint64_t messages{};
    for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::fprintf(stderr,"%s\n",message->pDescription);}}
    Require(messages==0,"debug-layer messages");
    // Exactness is required only on the adapter and driver that drew the capture (reference-tonemap-adapter.txt: vendor, device,
    // user-mode driver version; measured there: 0 differing bytes). Elsewhere (another GPU or driver, WARP, a capture without the
    // file) the arithmetic of exp, log and the filtering may differ: within 1/255 and on fewer than 5% of the bytes.
    std::string captured_adapter;{std::ifstream file(dir+"/reference-tonemap-adapter.txt");std::getline(file,captured_adapter);}
    std::string this_adapter;{DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);LARGE_INTEGER driver{};(void)adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice),&driver);
        char text[96]{};std::snprintf(text,sizeof(text),"%04X %04X %016llX",desc.VendorId,desc.DeviceId,static_cast<unsigned long long>(driver.QuadPart));this_adapter=text;}
    const bool same=!warp && !captured_adapter.empty() && captured_adapter==this_adapter;
    const uint64_t total=differing[0]+differing[1]+differing[2]+differing[3];
    if(same)Require(largest==0,"the native tone map differs from the original on the capture's adapter");
    else Require(largest<=1 && total*20<uint64_t(W)*H*4,"the native tone map differs from the original beyond its bound");
    std::printf("PASS tone-map stage %s: T12 %s: the original's tone-map inputs through our pass%s equal its output within %d/255 (%s), no debug-layer message\n",
        argv[1],dir.substr(dir.find_last_of("/\\")+1).c_str(),vignette?" (vignette variant, coordinates bit for bit)":"",largest,same?"the capture's adapter and driver: exactness required":"another adapter, driver or WARP: within 1/255");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
