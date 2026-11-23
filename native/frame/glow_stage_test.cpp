// W6 GL-5 T4/T5 (GLOW-WAVE1-READY 7): the glow stage with the original's four programmes (a directory holding the gxposteffect
// containers ScreenSpaceQuadShader.vpo, EmissiveToGlowPS.ppo, Sample13Weight.ppo, GlowCompositeSinglePS.ppo, as the I-0063
// programme cache names them; skipped with a message without it), on WARP or hardware with the debug layer.
// T4: the four blobs match their pins; one flipped byte does not. T5: a 64 x 36 RGBA16F GB5 with a 4 x 4 patch (1, 0.5, 0.25, 1)
// and an RGBA8 final target cleared to zero: after one Record with (blur 1, rgb 1.5, final 0.5) the final target equals a CPU
// model of the ten passes (bilinear clamp sampling, the 13 taps at one target texel, R11G11B10 storage after every glow write,
// the additive composites) within 2/255; ten passes; the scales at 0 and a GB5 alpha of 0 leave the final target unchanged;
// no debug-layer message of severity WARNING or worse.
#include "../renderer/glow_stage.h"
#include "../renderer/glow_math.h"
#include "../renderer/model_draw.h"
#include <filesystem>
#include "../content/acquire.h"
#include <dxgi1_6.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void Check(HRESULT result,const char* message){if(FAILED(result))throw std::runtime_error(message);}
std::vector<uint8_t> FileBytes(const std::string& path){
    std::ifstream file(path,std::ios::binary);Require(bool(file),"programme file unreadable");
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());}
bool Pinned(const std::vector<uint8_t>& bytes,const GlowPin& pin){char hex[65]{};
    return bytes.size()==pin.bytes && ds3content::Sha256(bytes.data(),bytes.size(),hex) && !std::strcmp(hex,pin.sha256);}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;}
uint16_t Half(float value){   // exact for the test's values (0, 0.25, 0.5, 1)
    if(value==0.f)return 0;int exponent{};const float mantissa=std::frexp(value,&exponent);
    return uint16_t(((exponent+14)<<10)|uint16_t((mantissa*2.f-1.f)*1024.f+0.5f));}
// R11G11B10_FLOAT storage of one non-negative channel (5-bit exponent, `bits` mantissa bits), rounded to nearest even.
float Small(float value,int bits){
    if(!(value>0.f))return 0.f;int exponent{};std::frexp(value,&exponent);   // value = m 2^exponent, m in [0.5, 1)
    const int e=std::max(exponent-1,-14);const float step=std::ldexp(1.f,e-bits);
    return std::min(std::nearbyint(value/step)*step,65024.f);}
struct Image {uint32_t w{},h{};std::vector<std::array<float,4>> p;
    Image(uint32_t width,uint32_t height):w(width),h(height),p(size_t(width)*height){}
    std::array<float,4>& at(uint32_t x,uint32_t y){return p[size_t(y)*w+x];}
    const std::array<float,4>& at(uint32_t x,uint32_t y) const {return p[size_t(y)*w+x];}
    std::array<float,4> Sample(float u,float v) const {   // bilinear, clamp, level 0
        const float x=u*float(w)-0.5f,y=v*float(h)-0.5f;const float fx=std::floor(x),fy=std::floor(y),ax=x-fx,ay=y-fy;
        const auto texel=[&](int tx,int ty){return at(uint32_t(std::clamp(tx,0,int(w)-1)),uint32_t(std::clamp(ty,0,int(h)-1)));};
        std::array<float,4> out{};const int ix=int(fx),iy=int(fy);
        for(int c=0;c<4;++c)out[c]=(texel(ix,iy)[c]*(1-ax)+texel(ix+1,iy)[c]*ax)*(1-ay)+(texel(ix,iy+1)[c]*(1-ax)+texel(ix+1,iy+1)[c]*ax)*ay;
        return out;}
    void Store(){for(auto& v:p){v[0]=Small(v[0],6);v[1]=Small(v[1],6);v[2]=Small(v[2],5);v[3]=1.f;}}
};
// The CPU model of the ten passes (GLOW-WAVE1-READY 3.3, orientation): returns the final RGBA8 colour.
std::vector<uint8_t> Model(const Image& gb5,const GlowValues& values){
    const uint32_t W=gb5.w,H=gb5.h;Image s(W,H);
    for(uint32_t y=0;y<H;++y)for(uint32_t x=0;x<W;++x){const auto t=gb5.at(x,y);s.at(x,y)={t[0]*t[3],t[1]*t[3],t[2]*t[3],1};}
    s.Store();
    uint8_t taps_bytes[256]{};glow::TapImage(values.blur_scale,taps_bytes);float taps[64]{};std::memcpy(taps,taps_bytes,256);
    std::vector<Image> levels;const Image* source=&s;
    for(unsigned level=0;level<4;++level){const auto size=glow::LevelSize(W,H,level);Image out(size.width,size.height);
        for(uint32_t y=0;y<size.height;++y)for(uint32_t x=0;x<size.width;++x){
            const float u=(float(x)+.5f)/float(size.width),v=(float(y)+.5f)/float(size.height);std::array<float,4> sum{};
            for(int i=0;i<13;++i){const auto t=source->Sample(u+taps[4*i]/float(size.width),v+taps[4*i+1]/float(size.height));
                for(int c=0;c<4;++c)sum[c]+=taps[4*i+2]*t[c];}
            out.at(x,y)=sum;}
        out.Store();levels.push_back(std::move(out));source=&levels.back();}
    const auto add=[&](Image& target,const Image& from,float scale){
        for(uint32_t y=0;y<target.h;++y)for(uint32_t x=0;x<target.w;++x){
            const auto t=from.Sample((float(x)+.5f)/float(target.w),(float(y)+.5f)/float(target.h));
            for(int c=0;c<3;++c)target.at(x,y)[c]+=t[c]*scale;}
        target.Store();};
    add(levels[2],levels[3],values.rgb_scale);add(levels[1],levels[2],values.rgb_scale);add(levels[0],levels[1],values.rgb_scale);add(s,levels[0],values.rgb_scale);
    std::vector<uint8_t> final(size_t(W)*H*4);
    for(uint32_t y=0;y<H;++y)for(uint32_t x=0;x<W;++x){const auto t=s.Sample((float(x)+.5f)/float(W),(float(y)+.5f)/float(H));
        for(int c=0;c<3;++c)final[(size_t(y)*W+x)*4+c]=uint8_t(std::lround(std::clamp(t[c]*values.final_scale,0.f,1.f)*255.f));
        final[(size_t(y)*W+x)*4+3]=255;}
    return final;}
struct Plain final:GlowRecorder {
    uint32_t passes{};
    void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to) override {
        const auto barrier=Transition(resource,from,to);list->ResourceBarrier(1,&barrier);}
    void Pass(const char*,ID3D12Resource*,ID3D12Resource*,bool) override {++passes;}
};
struct Rig {
    static constexpr UINT W=64,H=36;
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;uint64_t value{};HANDLE event{};ComPtr<ID3D12Resource> gb5,final,source,readback,constants;ComPtr<ID3D12DescriptorHeap> rtv;
    uint8_t* constant_memory{};
    explicit Rig(IDXGIAdapter1* adapter){
        Check(D3D12CreateDevice(adapter,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"control device");
        const D3D12_COMMAND_QUEUE_DESC queue_desc{D3D12_COMMAND_LIST_TYPE_DIRECT};Check(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)),"control queue");
        Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"control allocator");
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");list->Close();
        Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"control fence");event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=W;desc.Height=H;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        desc.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&gb5)),"control GB5");
        desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;D3D12_CLEAR_VALUE clear{DXGI_FORMAT_R8G8B8A8_UNORM,{0,0,0,0}};
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&final)),"control final");
        D3D12_DESCRIPTOR_HEAP_DESC heaps{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};Check(device->CreateDescriptorHeap(&heaps,IID_PPV_ARGS(&rtv)),"control rtv heap");
        device->CreateRenderTargetView(final.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
        source=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,512*H,D3D12_RESOURCE_STATE_GENERIC_READ);
        readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,256*H,D3D12_RESOURCE_STATE_COPY_DEST);
        constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,GlowStage::ConstantBytes,D3D12_RESOURCE_STATE_GENERIC_READ);
        const D3D12_RANGE none{};Check(constants->Map(0,&none,reinterpret_cast<void**>(&constant_memory)),"control constants map");}
    ~Rig(){if(event)CloseHandle(event);}
    void Fill(const Image& image){uint8_t* memory{};const D3D12_RANGE none{};Check(source->Map(0,&none,reinterpret_cast<void**>(&memory)),"control source map");
        for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x)for(UINT c=0;c<4;++c){const uint16_t h=Half(image.at(x,y)[c]);std::memcpy(memory+y*512+x*8+c*2,&h,2);}
        source->Unmap(0,nullptr);}
    // GB5 from the source rows, the final target cleared to zero, one Record, the final target read back (RGBA8, 256-byte rows).
    std::vector<uint8_t> Run(GlowStage& stage,const GlowValues& values,uint32_t& passes){
        Check(allocator->Reset(),"control allocator reset");Check(list->Reset(allocator.Get(),nullptr),"control list reset");
        auto barrier=Transition(gb5.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION to{gb5.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{source.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,W,H,1,512};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        barrier=Transition(gb5.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_RENDER_TARGET);list->ResourceBarrier(1,&barrier);
        const float zero[4]{};list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),zero,0,nullptr);
        Plain recorder;
        stage.Record(list.Get(),gb5.Get(),DXGI_FORMAT_R16G16B16A16_FLOAT,1,rtv->GetCPUDescriptorHandleForHeapStart(),values,
            [&](uint64_t bytes){Require(bytes==GlowStage::ConstantBytes,"constant request");return GlowConstants{constant_memory,constants->GetGPUVirtualAddress()};},recorder);
        passes=recorder.passes;
        barrier=Transition(final.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION out{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},image{final.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        out.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,W,H,1,256};list->CopyTextureRegion(&out,0,0,0,&image,nullptr);
        barrier=Transition(final.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);list->ResourceBarrier(1,&barrier);
        Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        Check(queue->Signal(fence.Get(),++value),"control signal");Check(fence->SetEventOnCompletion(value,event),"control wait");WaitForSingleObject(event,INFINITE);
        std::vector<uint8_t> pixels(size_t(W)*H*4);uint8_t* memory{};const D3D12_RANGE read{0,256*H};Check(readback->Map(0,&read,reinterpret_cast<void**>(&memory)),"control readback");
        for(UINT y=0;y<H;++y)std::memcpy(pixels.data()+y*W*4,memory+y*256,W*4);const D3D12_RANGE none{};readback->Unmap(0,&none);return pixels;}
    uint64_t DebugMessages() const {
        ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");uint64_t found{};
        for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
            auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
            if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++found;std::fprintf(stderr,"%s\n",message->pDescription);}}
        return found;}
};
}
int main(int argc,char** argv){try{
    Require(argc==3,"usage: glow_stage_test warp|hardware <programme directory>");const bool warp=!std::strcmp(argv[1],"warp");
    Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
    const std::string dir=argv[2];
    if(!std::ifstream(dir+"/gxposteffect__ScreenSpaceQuadShader.vpo")){std::printf("PASS glow stage %s: skipped, no programme directory\n",argv[1]);return 0;}
    GlowBlobs blobs{FileBytes(dir+"/gxposteffect__ScreenSpaceQuadShader.vpo"),FileBytes(dir+"/gxposteffect__EmissiveToGlowPS.ppo"),
        FileBytes(dir+"/gxposteffect__Sample13Weight.ppo"),FileBytes(dir+"/gxposteffect__GlowCompositeSinglePS.ppo")};
    // T4: the pins.
    Require(Pinned(blobs.vertex,glow_pins[0]) && Pinned(blobs.extract,glow_pins[1]) && Pinned(blobs.blur,glow_pins[2]) && Pinned(blobs.composite,glow_pins[3]),"T4 pins");
    {auto flipped=blobs.blur;flipped[100]^=1;Require(!Pinned(flipped,glow_pins[2]),"T4 a flipped byte passes the pin");}
    // T6: the Forward2 rule (WritesSecondTarget) on the original programmes: the one-output composite stays Forward1, and the
    // forward programmes of the cache that declare SV_Target1 are counted (at least one: the glow-writing forward families).
    {const auto inspect=[](const std::vector<uint8_t>& bytes){ds3content::CpuShaderProgram program;program.stage=ds3content::ShaderStage::Pixel;
            program.model_major=5;program.bytecode=bytes;return ds3content::InspectShaderInterface(program);};
        Require(!WritesSecondTarget(inspect(blobs.composite)),"T6 a one-output programme selects Forward2");
        size_t forward{},second{};
        for(const auto& entry:std::filesystem::directory_iterator(dir)){const auto name=entry.path().filename().string();
            if(name.size()<8 || name.compare(name.size()-8,8,"_Fwd.ppo"))continue;++forward;
            try{if(WritesSecondTarget(inspect(FileBytes(entry.path().string()))))++second;}catch(const std::exception&){}}
        std::printf("forward programmes: %zu, with SV_Target1: %zu\n",forward,second);Require(second>0,"T6 no forward programme writes GB5");}
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"control factory");
    ComPtr<IDXGIAdapter1> adapter;
    if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"control WARP adapter");
    else{Check(factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter)),"control hardware adapter");
        DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);Require(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE),"no hardware adapter");}
    Rig rig(adapter.Get());GlowStage stage;std::string reason;
    Require(stage.Open(rig.device.Get(),blobs,DXGI_FORMAT_R8G8B8A8_UNORM,Rig::W,Rig::H,reason),reason.c_str());
    Image gb5(Rig::W,Rig::H);for(UINT y=14;y<18;++y)for(UINT x=30;x<34;++x)gb5.at(x,y)={1.f,.5f,.25f,1.f};rig.Fill(gb5);
    // T5: the chain against the model.
    const GlowValues values{1.f,1.5f,.5f,0x3F800000u};uint32_t passes{};const auto pixels=rig.Run(stage,values,passes);
    const auto model=Model(gb5,values);int largest{};size_t lit{};
    for(size_t i=0;i<pixels.size();++i){largest=std::max(largest,std::abs(int(pixels[i])-int(model[i])));if(i%4!=3 && pixels[i])++lit;}
    std::printf("glow chain: largest difference %d/255, %zu lit channels\n",largest,lit);
    Require(largest<=2,"T5 the chain differs from the model");Require(lit>48,"T5 no glow around the patch");
    Require(passes==10 && stage.Passes()==10,"T5 ten passes");
    // T5: zero scales and a zero alpha leave the final target unchanged (zero colour, alpha written 1 by the composite).
    const auto zero_scale=rig.Run(stage,{1.f,0.f,0.f,0x3F800000u},passes);
    for(size_t i=0;i<zero_scale.size();++i)Require(i%4==3 || !zero_scale[i],"T5 zero scales change the colour");
    for(auto& texel:gb5.p)texel[3]=0.f;rig.Fill(gb5);const auto zero_alpha=rig.Run(stage,values,passes);
    for(size_t i=0;i<zero_alpha.size();++i)Require(i%4==3 || !zero_alpha[i],"T5 a zero GB5 alpha changes the colour");
    // A resize keeps the pipelines and recreates the targets (same extent here); the chain still runs.
    stage.Resize(Rig::W,Rig::H);rig.Run(stage,values,passes);Require(stage.Passes()==40,"T5 passes after the resize");
    Require(rig.DebugMessages()==0,"T5 debug-layer messages");
    std::printf("PASS glow stage %s: T4 the four original programmes match their pins and a flipped byte does not; T6 the Forward2 rule; T5 the ten passes equal the CPU model "
        "within %d/255 (R11G11B10 storage after each glow write), zero scales and a zero alpha change nothing, a resize keeps the chain, no debug-layer message\n",argv[1],largest);
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
