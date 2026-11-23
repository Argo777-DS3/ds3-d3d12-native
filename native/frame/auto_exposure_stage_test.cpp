// I-0132 R11h (the final review's M1): AutoExposureStage's kernels run on WARP against their CPU mirror (auto_exposure_math.h): a 64x64
// grey scene in two known luminances, the tone map's words of Firelink; frame 1 snaps (reset) to the solved factor, which the next
// frames keep (the goal unchanged); the state's factor and goal and the 1x1 factor texel equal SolveLog2's within 1e-3 stop; a black
// frame (no pixel in range) holds the factor; a reset on a black frame snaps at the next lit one; DLSS's exposure is the area's x a;
// no debug-layer message.
#include "../renderer/auto_exposure.h"
#include "../renderer/auto_exposure_math.h"
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace ds3native;
namespace {
void Check(HRESULT hr,const char* what){if(FAILED(hr))throw std::runtime_error(what);}
void Require(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
uint16_t Half(float f){uint32_t b;std::memcpy(&b,&f,4);const int e=int((b>>23)&0xFF)-127+15;return e<=0?0:e>=31?0x7BFF:uint16_t((e<<10)|((b>>13)&0x3FF));}
float HalfToFloat(uint16_t h){const int e=(h>>10)&0x1F;const float m=float(h&0x3FF);return e?std::ldexp(1.f+m/1024.f,e-15):std::ldexp(m/1024.f,-14);}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* d,uint64_t bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
    D3D12_HEAP_PROPERTIES h{};h.Type=type;D3D12_RESOURCE_DESC r{};r.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;r.Width=bytes;r.Height=1;r.DepthOrArraySize=1;
    r.MipLevels=1;r.SampleDesc.Count=1;r.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;ComPtr<ID3D12Resource> out;
    Check(d->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&r,state,nullptr,IID_PPV_ARGS(&out)),"test buffer");return out;}
void Barrier(ID3D12GraphicsCommandList* l,ID3D12Resource* r,D3D12_RESOURCE_STATES a,D3D12_RESOURCE_STATES b){
    D3D12_RESOURCE_BARRIER x{};x.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;x.Transition={r,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,a,b};l->ResourceBarrier(1,&x);}
}
int main(){try{
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"factory");
    ComPtr<IDXGIAdapter1> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"WARP adapter");
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"WARP device");
    ComPtr<ID3D12InfoQueue> info;Require(SUCCEEDED(device.As(&info)),"no debug-layer info queue (the message check would pass vacuously)");
    const D3D12_COMMAND_QUEUE_DESC qd{D3D12_COMMAND_LIST_TYPE_DIRECT};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)),"queue");
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");uint64_t tick=0;
    const auto submit=[&]{Check(list->Close(),"close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);Check(queue->Signal(fence.Get(),++tick),"signal");
        HANDLE e=CreateEventW(nullptr,FALSE,FALSE,nullptr);fence->SetEventOnCompletion(tick,e);WaitForSingleObject(e,INFINITE);CloseHandle(e);
        Check(allocator->Reset(),"allocator reset");Check(list->Reset(allocator.Get(),nullptr),"list reset");};
    // The scene: 64x64 R16G16B16A16_FLOAT, grey, the left half L0, the right half L1 (exact halves); a black scene beside it.
    constexpr uint32_t n=64;const float L0=HalfToFloat(Half(0.02f)),L1=HalfToFloat(Half(std::exp2(-0.9375f)));   // L1 at a bin's centre (no float edge)
    D3D12_HEAP_PROPERTIES dh{};dh.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=n;td.Height=n;
    td.DepthOrArraySize=1;td.MipLevels=1;td.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;td.SampleDesc.Count=1;
    ComPtr<ID3D12Resource> lit,black;
    Check(device->CreateCommittedResource(&dh,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&lit)),"scene");
    Check(device->CreateCommittedResource(&dh,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&black)),"black scene");
    auto upload=Buffer(device.Get(),2ull*n*n*8,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
    {uint16_t* p{};Check(upload->Map(0,nullptr,reinterpret_cast<void**>(&p)),"map");
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){const uint16_t v=Half(x<n/2?L0:L1);uint16_t* t=p+4*(y*n+x);t[0]=t[1]=t[2]=v;t[3]=Half(1.f);}
        std::memset(p+4*n*n,0,8*n*n);upload->Unmap(0,nullptr);}
    for(int k=0;k<2;++k){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        from.PlacedFootprint.Offset=uint64_t(k)*n*n*8;from.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,n,n,1,n*8};
        to.pResource=k?black.Get():lit.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        Barrier(list.Get(),to.pResource,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);}
    submit();
    // The tone map's words (Firelink's: exposure 2.3, gamma 2.2, the curve of FilmicContrast 0.3), the identity matrix.
    PostToneParams tone{};tone.exposure=2.3f;tone.inv_gamma=1.0f/2.2f;tone.curve_a=2.39853048f;tone.curve_b=0.514153361f;
    tone.colour[0][0]=tone.colour[1][1]=tone.colour[2][2]=1.0f;
    const auto words=exposure::ToneWordsOf(tone);
    auto constants=Buffer(device.Get(),64*4096,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);uint8_t* cpu{};Check(constants->Map(0,nullptr,reinterpret_cast<void**>(&cpu)),"map constants");
    uint64_t used=0;const AutoExposureConstantSink sink=[&](uint64_t bytes){if(used+bytes>64ull*4096)used=0;const uint64_t at=used;used=(used+bytes+255)/256*256;
        return AutoExposureConstantMemory{cpu+at,constants->GetGPUVirtualAddress()+at};};   // each frame waited: the ring's reuse is safe
    AutoExposureStage stage;std::string reason;Require(stage.Open(device.Get(),reason),reason.c_str());
    // The CPU mirror's answer for this scene.
    std::array<uint32_t,AutoExposureStage::bins> bins{};
    for(const float l:{L0,L1})bins[uint32_t((std::log2(l)-AutoExposureStage::log2_min)/AutoExposureStage::bin_width)]+=n*n/2;
    AutoExposureParams params{};params.target=0.20f;params.fixed_dt=1.0f/60.0f;   // a fixed step: the adaptation deterministic
    const float goal=exposure::SolveLog2(words,exposure::KeptWeights(bins,params.low,params.high),AutoExposureStage::log2_min,AutoExposureStage::bin_width,params.target,params.min_log2,params.max_log2);
    auto texel=Buffer(device.Get(),256,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    D3D12_RESOURCE_DESC ed=td;ed.Width=ed.Height=1;ed.Format=DXGI_FORMAT_R32_FLOAT;ComPtr<ID3D12Resource> dlss;
    Check(device->CreateCommittedResource(&dh,D3D12_HEAP_FLAG_NONE,&ed,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&dlss)),"DLSS exposure");
    auto dlss_read=Buffer(device.Get(),256,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto frame=[&](ID3D12Resource* scene,bool reset,bool read_factor){params.reset=reset;stage.Record(list.Get(),scene,0,tone,params,sink);
        if(read_factor){Barrier(list.Get(),stage.Factor(),D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
            D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=stage.Factor();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            to.pResource=texel.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint.Footprint={DXGI_FORMAT_R32G32B32A32_FLOAT,1,1,1,256};
            list->CopyTextureRegion(&to,0,0,0,&from,nullptr);Barrier(list.Get(),stage.Factor(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            Require(stage.CopyDlssExposure(list.Get(),dlss.Get()),"DLSS exposure copy");
            Barrier(list.Get(),dlss.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COPY_SOURCE);
            D3D12_TEXTURE_COPY_LOCATION a{},b{};a.pResource=dlss.Get();a.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;b.pResource=dlss_read.Get();
            b.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;b.PlacedFootprint.Footprint={DXGI_FORMAT_R32_FLOAT,1,1,1,256};list->CopyTextureRegion(&b,0,0,0,&a,nullptr);
            Barrier(list.Get(),dlss.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_DEST);}
        submit();};
    const auto read=[](ID3D12Resource* r){float v[4]{};void* p{};const D3D12_RANGE range{0,16};Check(r->Map(0,&range,&p),"map readback");std::memcpy(v,p,16);r->Unmap(0,nullptr);return std::array<float,4>{v[0],v[1],v[2],v[3]};};
    // Frame 1 snaps; frames 2 to 10 keep it; the reading of frame 2 arrives at frame 10 (eight frames late).
    frame(lit.Get(),true,false);for(int k=0;k<9;++k)frame(lit.Get(),false,k==8);
    auto r=stage.LastReading();
    Require(r.valid && std::abs(r.goal_log2-goal)<1e-3f && std::abs(r.log2_factor-goal)<1e-3f,"the state's goal and factor are the CPU mirror's (SolveLog2)");
    Require(std::abs(r.mean-params.target)<2e-3f && r.pixels==n*n && std::abs(r.area_exposure-2.3f)<1e-6f,"the mean at the factor is the target, every pixel counted");
    const auto t=read(texel.Get());Require(std::abs(std::log2(t[0])-goal)<1e-3f && t[0]==t[1] && t[1]==t[2] && t[3]==1.f,"the factor texel is (a, a, a, 1)");
    Require(std::abs(read(dlss_read.Get())[0]-2.3f*t[0])<1e-4f*t[0],"DLSS's exposure is the area's x a");
    // A black frame holds the factor; a reset on a black frame snaps at the next lit frame.
    for(int k=0;k<9;++k)frame(black.Get(),k==0,false);   // frame 11 a reset on black, then black
    r=stage.LastReading();Require(r.valid && std::abs(r.log2_factor-goal)<1e-3f && r.pixels==0,"a black frame holds the factor (no pixel in range)");
    params.target=0.35f;const float bright=exposure::SolveLog2(words,exposure::KeptWeights(bins,params.low,params.high),AutoExposureStage::log2_min,AutoExposureStage::bin_width,params.target,params.min_log2,params.max_log2);
    frame(black.Get(),true,false);for(int k=0;k<9;++k)frame(lit.Get(),false,false);   // the reset kept until the first lit frame
    r=stage.LastReading();Require(r.valid && std::abs(r.log2_factor-bright)<1e-3f,"a reset on a black frame snaps at the next lit frame");
    // The pending reset cleared: back to 0.20 without a reset, the next lit frame steps towards the goal by the law's step for 1/60 s (no snap).
    params.target=0.20f;for(int k=0;k<9;++k)frame(lit.Get(),false,false);
    r=stage.LastReading();Require(r.valid && std::abs(r.log2_factor-exposure::Adapt(bright,goal,1.0f/60.0f,params.speed_brighter,params.speed_darker,params.transition))<1e-3f,
        "the reset cleared: the next lit frame adapts by the CPU mirror's step (not a snap)");
    // I-0138 part 4c: a Reinhard block (tone type 3, F 32: c208 = (4/1024 - 2, 1, 1/1024, 512)): the kernels model its curve, as the
    // CPU mirror does; its goal differs from the sensitometric one's.
    PostToneParams reinhard=tone;reinhard.flags|=kPostToneReinhard;reinhard.curve_a=-1.99609375f;reinhard.curve_b=1.0f;reinhard.curve_c=0.0009765625f;
    const float reinhard_goal=exposure::SolveLog2(exposure::ToneWordsOf(reinhard),exposure::KeptWeights(bins,params.low,params.high),AutoExposureStage::log2_min,
        AutoExposureStage::bin_width,params.target,params.min_log2,params.max_log2);
    tone=reinhard;frame(lit.Get(),true,false);for(int k=0;k<9;++k)frame(lit.Get(),false,false);
    r=stage.LastReading();Require(r.valid && std::abs(r.goal_log2-reinhard_goal)<1e-3f && std::abs(r.log2_factor-reinhard_goal)<1e-3f && std::abs(r.mean-params.target)<2e-3f,
        "the Reinhard block: the kernels' goal and factor are the CPU mirror's, the mean at the factor the target");
    Require(std::abs(reinhard_goal-goal)>1e-2f,"the Reinhard curve's goal differs from the sensitometric one's (the curve is read)");
    // I-0138 tranche 5e (its step review's L3): the game's lower bound of the factor (log2 a >= 0) in the kernels: this scene is
    // brighter than the target (its free factor under 1, above), so a reset snaps to the factor 1 and the mean stays over the target.
    params.min_log2=kAutoExposureMinLog2;frame(lit.Get(),true,false);for(int k=0;k<9;++k)frame(lit.Get(),false,k==8);
    r=stage.LastReading();const auto bounded=read(texel.Get());
    Require(reinhard_goal<-1.0f && r.valid && std::abs(r.goal_log2)<1e-3f && std::abs(r.log2_factor)<1e-3f && r.mean>params.target+0.05f,
        "the game's lower bound: a bright scene keeps the factor 1 (the goal and the factor 0 in log2, the mean over the target)");
    Require(std::abs(bounded[0]-1.0f)<1e-3f && std::abs(read(dlss_read.Get())[0]-2.3f)<1e-3f,"the game's lower bound: the factor texel 1, DLSS's exposure the area's");
    Require(info->GetNumStoredMessages()==0,"debug-layer messages");
    std::printf("PASS auto exposure stage (WARP): the kernels' goal, factor and mean equal the CPU mirror's (log2 a %.4f; the Reinhard curve's %.4f), the factor texel and DLSS's exposure, "
        "a black frame holds, a reset on a black frame snaps at the next lit one, no debug-layer message\n",goal,reinhard_goal);
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
