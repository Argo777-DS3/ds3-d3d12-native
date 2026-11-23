// I-0132 R11j: TaaStage on WARP with the debug layer. A 32x32 checkerboard whose phase flips every frame (a pixel 0 then 1: the aliasing a
// jittered edge makes) resolves to a steady grey (every pixel within [0.2, 0.6] after 40 frames, their spread under a quarter of the
// input's), as a uniform scene stays itself; a reset frame and a reprojection off the screen give the current colour alone (the
// shader's reconstruction, mirrored here, with and without a jitter: a vertical and a horizontal edge); the sharpening steepens an edge
// within its range, keeps a highlight within its neighbours' and leaves a uniform scene; the motion's sign; the clip's colour inverse;
// a jittered edge converging; an infinite sample bounded; a wrong input refused; a dissolve's hole taking the current colour (I-0113
// tranche 41: the reactive weight over the 3x3) and its view; a fading row's dots and holes the current frame's own samples where the
// fade mask marks them (I-0140, gap 23); no debug-layer message.
#include "../renderer/taa_stage.h"
#include "../renderer/upscaler.h"
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
uint16_t Half(float f){if(f==0.0f)return 0;uint32_t b;std::memcpy(&b,&f,4);const int e=int((b>>23)&0xFF)-127+15;return e<=0?0:e>=31?0x7BFF:uint16_t((e<<10)|((b>>13)&0x3FF));}
float HalfToFloat(uint16_t h){const int e=(h>>10)&0x1F;const float m=float(h&0x3FF);const float v=e?std::ldexp(1.f+m/1024.f,e-15):std::ldexp(m/1024.f,-14);return (h&0x8000)?-v:v;}
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
    constexpr uint32_t n=32;const auto psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,npsr=D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    D3D12_HEAP_PROPERTIES dh{};dh.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_HEAP_PROPERTIES uh{};uh.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_HEAP_PROPERTIES rh{};rh.Type=D3D12_HEAP_TYPE_READBACK;
    const auto texture=[&](DXGI_FORMAT format,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,const D3D12_CLEAR_VALUE* clear){
        D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=n;td.Height=n;td.DepthOrArraySize=1;td.MipLevels=1;td.Format=format;td.SampleDesc.Count=1;td.Flags=flags;
        ComPtr<ID3D12Resource> r;Check(device->CreateCommittedResource(&dh,D3D12_HEAP_FLAG_NONE,&td,state,clear,IID_PPV_ARGS(&r)),"test texture");return r;};
    const auto buffer=[&](const D3D12_HEAP_PROPERTIES& h,uint64_t bytes,D3D12_RESOURCE_STATES state){
        D3D12_RESOURCE_DESC r{};r.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;r.Width=bytes;r.Height=1;r.DepthOrArraySize=1;r.MipLevels=1;r.SampleDesc.Count=1;r.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> out;Check(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&r,state,nullptr,IID_PPV_ARGS(&out)),"test buffer");return out;};
    // The colours, R16G16B16A16_FLOAT, rows of 256 bytes (32 x 8): 0 and 1 the checkerboard in its two phases; 2 a uniform grey; 3 the
    // grey with one +Inf sample; 4 a uniform 1 with one 4 at (16, 16) (the sharpening's bound); 5 a square of 1 on 0 (x 8 to 11, y 8 to
    // 23) and 6 the same moved 3 pixels right (the motion's sign); 7 a uniform red and 8 a uniform green (the clip's colour inverse);
    // 9 to 16 a vertical edge at x = 16.5 drawn under the 8 Halton phases' jitter (the scene shifted by +jitter: a pixel's centre x +
    // 0.5 sees x + 0.5 - jx; the convergence to the unjittered edge); 17 a horizontal edge at y = 16.5 under phase 6's jy. The final
    // review's M1. I-0113 tranche 41: 18 black (the reactive base), 19 a square of 1 (x and y 8 to 23) dissolved to every other pixel
    // ((x + y) even) on black, 20 the same square whole, 21 the checkerboard's phase 0 with (16, 16) at 0.5.
    constexpr uint32_t kinds=22;
    const auto texel=[&](uint32_t k,uint32_t x,uint32_t y)->std::array<float,3>{
        if(k<=1){const float v=float(((x+y+k)&1u));return {v,v,v};}
        if(k<=3)return {0.4f,0.4f,0.4f};
        if(k==4){const float v=x==16 && y==16?4.0f:1.0f;return {v,v,v};}
        if(k<=6){const uint32_t left=k==5?8:11;const float v=x>=left && x<left+4 && y>=8 && y<24?1.0f:0.0f;return {v,v,v};}
        if(k==7)return {0.5f,0.0f,0.0f};if(k==8)return {0.0f,0.5f,0.0f};
        if(k==21){const float v=x==16 && y==16?0.5f:float((x+y)&1u);return {v,v,v};}
        if(k>=18){const bool in=x>=8 && x<24 && y>=8 && y<24;const float v=k==18 || !in || (k==19 && ((x+y)&1u))?0.0f:1.0f;return {v,v,v};}
        if(k==17){const float jy=ds3native::upscaler::Jitter(6,8)[1];const float v=float(y)+0.5f-jy<16.5f?1.0f:0.0f;return {v,v,v};}
        const float jx=ds3native::upscaler::Jitter(k-9,8)[0];const float v=float(x)+0.5f-jx<16.5f?1.0f:0.0f;return {v,v,v};};
    std::vector<ComPtr<ID3D12Resource>> colours;auto upload=buffer(uh,uint64_t(kinds)*n*n*8,D3D12_RESOURCE_STATE_GENERIC_READ);
    {uint16_t* p{};Check(upload->Map(0,nullptr,reinterpret_cast<void**>(&p)),"map");
        for(uint32_t k=0;k<kinds;++k)for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){
            const auto v=texel(k,x,y);uint16_t* t=p+(size_t(k)*n*n+size_t(y)*n+x)*4;t[0]=Half(v[0]);t[1]=Half(v[1]);t[2]=Half(v[2]);t[3]=Half(1.f);
            if(k==3 && x==16 && y==16)t[0]=t[1]=t[2]=0x7C00;}   // the step review's M2: one +Inf sample (an fp16 overflow)
        upload->Unmap(0,nullptr);}
    for(uint32_t k=0;k<kinds;++k){colours.push_back(texture(DXGI_FORMAT_R16G16B16A16_FLOAT,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST,nullptr));
        D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=upload.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        from.PlacedFootprint.Offset=uint64_t(k)*n*n*8;from.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,n,n,1,n*8};
        to.pResource=colours.back().Get();to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        Barrier(list.Get(),to.pResource,D3D12_RESOURCE_STATE_COPY_DEST,psr|npsr);}
    // The depth at 0.5, two motion targets (still, and 1000 pixels to the right: off the screen), the output.
    D3D12_CLEAR_VALUE dc{};dc.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;dc.DepthStencil.Depth=0.5f;
    // The scene's depth is typeless (DepthTarget: R32G8X24_TYPELESS, its views typed; the step review's H1).
    auto depth=texture(DXGI_FORMAT_R32G8X24_TYPELESS,D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,D3D12_RESOURCE_STATE_DEPTH_WRITE,&dc);
    D3D12_CLEAR_VALUE mc{};mc.Format=DXGI_FORMAT_R16G16_FLOAT;
    auto still=texture(DXGI_FORMAT_R16G16_FLOAT,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,&mc);
    D3D12_CLEAR_VALUE ac=mc;ac.Color[0]=1000.f;   // its own clear value (the debug layer warns on a clear that differs)
    auto away=texture(DXGI_FORMAT_R16G16_FLOAT,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,&ac);
    D3D12_CLEAR_VALUE bc=mc;bc.Color[0]=-3.f;   // previous minus current: the scene moved 3 pixels right since the last frame
    auto back3=texture(DXGI_FORMAT_R16G16_FLOAT,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,&bc);
    auto output=texture(DXGI_FORMAT_R16G16B16A16_FLOAT,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr);
    {ComPtr<ID3D12DescriptorHeap> rtvs,dsvs;D3D12_DESCRIPTOR_HEAP_DESC hd{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,3,D3D12_DESCRIPTOR_HEAP_FLAG_NONE,0};
        Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtvs)),"rtvs");hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;hd.NumDescriptors=1;Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&dsvs)),"dsvs");
        auto rtv=rtvs->GetCPUDescriptorHandleForHeapStart();const auto stride=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        device->CreateRenderTargetView(still.Get(),nullptr,rtv);const float zero[4]{};list->ClearRenderTargetView(rtv,zero,0,nullptr);
        rtv.ptr+=stride;device->CreateRenderTargetView(away.Get(),nullptr,rtv);const float far_right[4]{1000.f,0,0,0};list->ClearRenderTargetView(rtv,far_right,0,nullptr);
        rtv.ptr+=stride;device->CreateRenderTargetView(back3.Get(),nullptr,rtv);const float left3[4]{-3.f,0,0,0};list->ClearRenderTargetView(rtv,left3,0,nullptr);
        Barrier(list.Get(),back3.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,npsr);
        const auto dsv=dsvs->GetCPUDescriptorHandleForHeapStart();D3D12_DEPTH_STENCIL_VIEW_DESC dd{};dd.Format=DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        dd.ViewDimension=D3D12_DSV_DIMENSION_TEXTURE2D;device->CreateDepthStencilView(depth.Get(),&dd,dsv);
        list->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,0.5f,0,0,nullptr);
        Barrier(list.Get(),still.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,npsr);Barrier(list.Get(),away.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,npsr);
        Barrier(list.Get(),depth.Get(),D3D12_RESOURCE_STATE_DEPTH_WRITE,D3D12_RESOURCE_STATE_DEPTH_READ|npsr);
        submit();}
    TaaStage taa;std::string reason;Require(taa.Open(device.Get(),reason),reason.c_str());
    auto readback=buffer(rh,uint64_t(n)*n*8,D3D12_RESOURCE_STATE_COPY_DEST);
    // One frame: the colour k with the motion, reset or not, the jitter it was drawn with; returns the output's red channel (all three
    // kept in `last`).
    std::vector<std::array<float,3>> last(size_t(n)*n);
    const auto frame=[&](uint32_t k,ID3D12Resource* motion,bool reset,uint32_t slot,float jx=0.0f,float jy=0.0f,ID3D12Resource* base=nullptr,bool view=false,
        ID3D12Resource* fade=nullptr,float kernel=2.29f,bool own=false){
        TaaParams p{};p.reset=reset;p.exposure=1.0f;p.jitter_x=jx;p.jitter_y=jy;p.base=base;p.reactive_view=view;p.fade_mask=fade;p.kernel=kernel;p.own_reset=own;
        Require(taa.Prepare(colours[k].Get(),depth.Get(),motion,output.Get(),reason),reason.c_str());
        taa.Record(list.Get(),colours[k].Get(),depth.Get(),motion,output.Get(),p,slot);
        Barrier(list.Get(),output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=output.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,n,n,1,n*8};
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        Barrier(list.Get(),output.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        submit();
        std::vector<float> red(size_t(n)*n);const uint16_t* q{};Check(readback->Map(0,nullptr,reinterpret_cast<void**>(const_cast<uint16_t**>(&q))),"readback map");
        for(size_t i=0;i<red.size();++i){red[i]=HalfToFloat(q[i*4]);last[i]={red[i],HalfToFloat(q[i*4+1]),HalfToFloat(q[i*4+2])};}
        readback->Unmap(0,nullptr);return red;};
    // The shader's reconstruction of the current colour at a pixel's centre (no jitter): the clamped 3x3, Gaussian by distance, each
    // sample also weighted 1 / (1 + its value) (exposure 1), as shaders/taa.hlsl.
    // The same with the frame's jitter (the samples' scene positions shifted by -jitter: the step review's M-a), on any colour.
    const auto current_j=[&](uint32_t k,int x,int y,float jx,float jy,double kernel=2.29){double sum=0,weight=0;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){const uint32_t qx=uint32_t((std::min)((std::max)(x+dx,0),int(n)-1)),qy=uint32_t((std::min)((std::max)(y+dy,0),int(n)-1));
            const float v=texel(k,qx,qy)[0];const double ex=dx-jx,ey=dy-jy;const double w=std::exp(-kernel*(ex*ex+ey*ey))/(1.0+v);sum+=v*w;weight+=w;}
        return float(sum/weight);};
    const auto current=[&](uint32_t k,int x,int y){double sum=0,weight=0;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){const int qx=(std::min)((std::max)(x+dx,0),int(n)-1),qy=(std::min)((std::max)(y+dy,0),int(n)-1);
            const float v=k==2?0.4f:float((uint32_t(qx+qy)+k)&1u);const double w=std::exp(-2.29*double(dx*dx+dy*dy))/(1.0+v);sum+=v*w;weight+=w;}
        return float(sum/weight);};
    const auto matches_current=[&](const std::vector<float>& r,uint32_t k){float worst=0;
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x)worst=(std::max)(worst,std::fabs(r[size_t(y)*n+x]-current(k,int(x),int(y))));
        return worst<0.004f;};
    const auto spread=[](const std::vector<float>& v,float& lo,float& hi){double s=0,s2=0;lo=1e9f;hi=-1e9f;
        for(const float x:v){s+=x;s2+=double(x)*x;lo=(std::min)(lo,x);hi=(std::max)(hi,x);}const double m=s/double(v.size());return float(std::sqrt((std::max)(0.0,s2/double(v.size())-m*m)));};
    // 1. The uniform grey stays itself (reset, then still frames).
    {auto r=frame(2,still.Get(),true,0);for(int i=0;i<5;++i)r=frame(2,still.Get(),false,uint32_t(i%3));float lo{},hi{};spread(r,lo,hi);
        Require(std::fabs(lo-0.4f)<0.005f && std::fabs(hi-0.4f)<0.005f,"T1 a uniform scene resolves to itself");}
    // 2. The flipping checkerboard: a reset frame is the current colour alone (the centre-weighted 3x3: near 1 or near 0), then 40
    //    frames resolve to a steady grey.
    {const auto first=frame(0,still.Get(),true,0);float lo{},hi{};const float input=spread(first,lo,hi);
        Require(matches_current(first,0),"T2 a reset frame is the current colour alone (no history)");
        std::vector<float> r;for(uint32_t i=1;i<=40;++i)r=frame(i&1u,still.Get(),false,i%3u);
        const float resolved=spread(r,lo,hi);std::printf("T2: input spread %.3f, resolved %.3f in [%.3f, %.3f]\n",input,resolved,lo,hi);
        Require(lo>0.2f && hi<0.6f && resolved<0.25f*input,"T2 the flipping checkerboard resolves to a steady grey");
        // 3. A reprojection off the screen takes the current colour alone, as a reset does.
        const auto off=frame(1,away.Get(),false,0);Require(matches_current(off,1),"T3 an off-screen reprojection is the current colour alone");}
    // 4. The sharpening (on a reset frame, the checkerboard's current colour): a uniform scene unchanged, the checkerboard's contrast
    //    raised, the history left unsharpened (the next frame without sharpening blends from it as before).
    {const auto frame_sharp=[&](uint32_t k,float amount){TaaParams p{};p.reset=true;p.exposure=1.0f;p.sharpen=amount;
            Require(taa.Prepare(colours[k].Get(),depth.Get(),still.Get(),output.Get(),reason),reason.c_str());
            taa.Record(list.Get(),colours[k].Get(),depth.Get(),still.Get(),output.Get(),p,0);
            Barrier(list.Get(),output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
            D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=output.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,n,n,1,n*8};
            list->CopyTextureRegion(&to,0,0,0,&from,nullptr);Barrier(list.Get(),output.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);submit();
            std::vector<float> red(size_t(n)*n);const uint16_t* q{};Check(readback->Map(0,nullptr,reinterpret_cast<void**>(const_cast<uint16_t**>(&q))),"readback map");
            for(size_t i=0;i<red.size();++i)red[i]=HalfToFloat(q[i*4]);readback->Unmap(0,nullptr);return red;};
        float lo{},hi{};const auto grey=frame_sharp(2,1.0f);spread(grey,lo,hi);Require(std::fabs(lo-0.4f)<0.005f && std::fabs(hi-0.4f)<0.005f,"T4 the sharpening leaves a uniform scene");
        // The square's edge (row 16), softened by the reconstruction: steepened (inside up, outside down) and bounded (the final review's
        // H1 and M1).
        const auto soft=frame_sharp(5,0.0f),sharp=frame_sharp(5,1.0f);const size_t in=16*n+8,out=16*n+7;
        std::printf("T4: the square's edge %.3f / %.3f, sharpened %.3f / %.3f\n",soft[out],soft[in],sharp[out],sharp[in]);
        // (the outer pixel's cross touches pure black, where the sharpening's amplitude is 0 by design: it must only not rise)
        Require(sharp[in]>soft[in]+0.01f && sharp[out]<=soft[out]+1e-3f,"T4 the sharpening steepens an edge");
        for(const float x:sharp)Require(x>=-1e-3f && x<=1.001f,"T4 the sharpened edge within its range");
        Require(matches_current(frame(0,still.Get(),true,1),0),"T4 the output unsharpened without it");
        // The history unsharpened (the second step review's L-b): a sharpened reset then a plain still frame equals a plain reset then
        // the same plain frame.
        (void)frame_sharp(0,1.0f);const auto after_sharp=frame(1,still.Get(),false,1);
        (void)frame(0,still.Get(),true,2);const auto after_plain=frame(1,still.Get(),false,0);
        float worst=0;for(size_t i=0;i<after_sharp.size();++i)worst=(std::max)(worst,std::fabs(after_sharp[i]-after_plain[i]));
        Require(worst<1e-3f,"T4 the history left unsharpened");}
    // 5. One +Inf sample (an fp16 overflow in the lit HDR): every output finite, its neighbours not black (the sample bounded, not a
    //    NaN spread over its 3x3), then a still frame keeps them so (the history not blackened).
    {for(int i=0;i<2;++i){const auto r=frame(3,still.Get(),i==0,uint32_t(i));
            for(const float x:r)Require(std::isfinite(x),"T5 an infinite sample gives finite outputs");
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)Require(r[size_t(16+dy)*n+size_t(16+dx)]>0.3f,"T5 an infinite sample does not blacken its neighbourhood");}}
    // 6. A depth of another format is refused by Prepare (no barrier recorded, the caller latches), not thrown at Record.
    {auto colour_only=texture(DXGI_FORMAT_R16G16B16A16_FLOAT,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COMMON,nullptr);std::string why;
        Require(!taa.Prepare(colours[2].Get(),colour_only.Get(),still.Get(),output.Get(),why) && !why.empty(),"T6 a wrong depth refused by Prepare");}
    // 8. The sharpening's bound (the final review's H1): a highlight of 4 among 1s at the strongest sharpening stays within [1, 4].
    {TaaParams p{};p.reset=true;p.exposure=1.0f;p.sharpen=1.0f;
        (void)frame(4,still.Get(),true,1);   // the reset frame without sharpening, its output read below as the reference
        const auto plain=last;
        Require(taa.Prepare(colours[4].Get(),depth.Get(),still.Get(),output.Get(),reason),reason.c_str());taa.Record(list.Get(),colours[4].Get(),depth.Get(),still.Get(),output.Get(),p,2);
        Barrier(list.Get(),output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=output.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        to.pResource=readback.Get();to.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;to.PlacedFootprint.Footprint={DXGI_FORMAT_R16G16B16A16_FLOAT,n,n,1,n*8};
        list->CopyTextureRegion(&to,0,0,0,&from,nullptr);Barrier(list.Get(),output.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);submit();
        const uint16_t* q{};Check(readback->Map(0,nullptr,reinterpret_cast<void**>(const_cast<uint16_t**>(&q))),"readback map");float lo=1e9f,hi=-1e9f;
        for(size_t i=0;i<size_t(n)*n;++i){const float v=HalfToFloat(q[i*4]);lo=(std::min)(lo,v);hi=(std::max)(hi,v);}readback->Unmap(0,nullptr);
        float plain_hi=0;for(const auto& v:plain)plain_hi=(std::max)(plain_hi,v[0]);
        std::printf("T8: a highlight of 4 among 1s sharpened at 1 within [%.3f, %.3f] (unsharpened peak %.3f)\n",lo,hi,plain_hi);
        Require(lo>=0.99f && hi<=4.01f,"T8 the sharpening keeps a highlight within its cross's range");}
    // 9. The motion's sign (previous minus current, in pixels): the square moved 3 pixels right, its left edge (x 11) reprojected into
    //    the square of the last frame (x 8), so its history is 1 there; with the sign reversed it would read the background.
    {(void)frame(5,still.Get(),true,0);const auto moved=frame(6,back3.Get(),false,1);
        std::printf("T9: the moved square's left edge %.3f\n",moved[16*n+11]);
        Require(moved[16*n+11]>0.5f && moved[16*n+14]>0.5f && moved[16*n+9]<0.1f,"T9 the motion's sign: the history follows the square");}
    // 10. The clip's colour inverse: a red history under a green frame (the box green alone) ends green after one frame.
    {(void)frame(7,still.Get(),true,0);(void)frame(8,still.Get(),false,1);const auto& v=last[16*n+16];
        Require(v[0]<0.02f && std::fabs(v[1]-0.5f)<0.01f && v[2]<0.02f,"T10 a red history clipped to the green frame");}
    // 11. A vertical edge drawn under the 8 Halton phases (the camera's jitter) converges to the unjittered edge: the pixel the edge
    //     crosses grey, its neighbours the two sides (a reversed jitter sign or reconstruction would smear or shift it).
    {std::vector<float> r;for(uint32_t s=0;s<48;++s){const auto j=ds3native::upscaler::Jitter(s,8);r=frame(9+(s%8),still.Get(),s==0,s%3,j[0],j[1]);}
        std::printf("T11: the edge's row %.3f %.3f | %.3f | %.3f %.3f\n",r[16*n+14],r[16*n+15],r[16*n+16],r[16*n+17],r[16*n+18]);
        Require(r[16*n+14]>0.9f && r[16*n+15]>0.6f && r[16*n+16]>0.1f && r[16*n+16]<0.7f && r[16*n+17]<0.35f && r[16*n+18]<0.1f,"T11 the jittered edge converges to the unjittered one");}
    // 12. One reset frame drawn under a jitter (phase 6: jx 0.375, jy 1/18) equals the jittered reconstruction mirrored here, for a vertical
    //     and a horizontal edge (a reversed or ignored jitter, in x or in y, would not: the step review's M-a).
    {const auto j=ds3native::upscaler::Jitter(6,8);
        for(const uint32_t k:{15u,17u}){const auto r=frame(k,still.Get(),true,0,j[0],j[1]);float worst=0;
            for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x)worst=(std::max)(worst,std::fabs(r[size_t(y)*n+x]-current_j(k,int(x),int(y),j[0],j[1])));
            std::printf("T12: the jittered reconstruction of colour %u within %.4f\n",k,worst);Require(worst<0.004f,"T12 a jittered frame's reconstruction at the unjittered centres");}}
    Require(taa.Frames()==114 && taa.Resets()==16,"T7 the stage's counters (114 frames, 16 resets)");
    // 13. I-0113 tranche 41: the reactive weighting taken over the 3x3. The whole square in the history, then the dissolved square over
    //     the black base (the frame before the 0x29 rows): a hole (16, 17), which the rows left black beside drawn dots, no longer keeps
    //     the square's history (the variance box of its 3x3, spanning 0 and 1, does not clip it); without the base it keeps it. The view
    //     paints the weight taken red in the output only: the next frame equals the one after the plain reactive frame.
    {const size_t hole=17*n+16,away_px=2*n+2;const float now=current_j(19,16,17,0.0f,0.0f);
        (void)frame(20,still.Get(),true,0);const auto plain=frame(19,still.Get(),false,1);
        (void)frame(20,still.Get(),true,2);const auto reactive=frame(19,still.Get(),false,0,0.0f,0.0f,colours[18].Get());
        const auto next_plain=frame(19,still.Get(),false,1,0.0f,0.0f,colours[18].Get());
        (void)frame(20,still.Get(),true,2);(void)frame(19,still.Get(),false,0,0.0f,0.0f,colours[18].Get(),true);const auto view=last[hole];const auto view_away=last[away_px];
        const auto next_view=frame(19,still.Get(),false,1,0.0f,0.0f,colours[18].Get());
        std::printf("T13: a hole of the dissolve %.3f without the base, %.3f with it (its current colour %.3f); the view (%.3f, %.3f, %.3f)\n",plain[hole],reactive[hole],now,view[0],view[1],view[2]);
        Require(plain[hole]>0.8f,"T13 without the base the hole keeps the square's history");
        Require(reactive[hole]<0.3f && reactive[hole]>now-0.01f,"T13 the hole beside drawn dots takes the current colour");
        Require(reactive[away_px]<1e-3f,"T13 the black background unchanged");
        Require(view[0]>0.85f && view[1]<0.05f && view_away[0]<1e-3f,"T13 the view paints the weight taken red");
        float worst=0;for(size_t i=0;i<next_view.size();++i)worst=(std::max)(worst,std::fabs(next_view[i]-next_plain[i]));
        Require(worst<1e-3f,"T13 the view leaves the history");
        // The radius (the Fable review's L2): the checkerboard in the history (phase 1), then phase 0 with one pixel (16, 16) changed
        // against its base (phase 0 itself): the weight is taken at (17, 16), one pixel away, and not at (18, 16), two away, which
        // resolves exactly as without the base (the box spans 0 and 1 there: the history kept).
        (void)frame(1,still.Get(),true,0);const auto ring_plain=frame(21,still.Get(),false,1);
        (void)frame(1,still.Get(),true,2);const auto ring=frame(21,still.Get(),false,0,0.0f,0.0f,colours[0].Get());
        std::printf("T13: one changed pixel: one away %.3f (%.3f without the base), two away %.3f (%.3f)\n",ring[16*n+17],ring_plain[16*n+17],ring[16*n+18],ring_plain[16*n+18]);
        Require(std::fabs(ring[16*n+17]-ring_plain[16*n+17])>0.1f,"T13 the weight taken one pixel from a change");
        Require(std::fabs(ring[16*n+18]-ring_plain[16*n+18])<1e-3f && std::fabs(ring[16*n+14]-ring_plain[16*n+14])<1e-3f,"T13 the weight kept two pixels from a change");
        Require(taa.ReactiveFrames()==5 && taa.ViewFrames()==1,"T13 the reactive and view frames counted");}
    // 14. I-0140 (gap 23): the fade mask (kTaaFadeMaskFormat, 0 at the dissolved square's drawn dots, (x + y) even inside it, 1
    //     elsewhere). The whole square in the history, then the dissolved square over the black base with the mask (jittered, phase 3):
    //     a dot (16, 16) is its own sample, 1, where the reconstruction alone softens it (tranche 41's form); a hole (16, 17) its own
    //     sample, 0 (no history left); an unmarked mask (all 1) leaves the whole frame as the base alone. The radius (its final
    //     review's M2): the checkerboard's phase 1 in the history, then phase 0 over itself as the base (no reactive change), jittered,
    //     with only (16, 16) marked: (17, 16), one away, is its own sample, 1; (18, 16), two away, resolves bit for bit as without the
    //     mask, a value its own sample (0) is not.
    {auto mask=texture(kTaaFadeMaskFormat,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST,nullptr);
        auto unmarked=texture(kTaaFadeMaskFormat,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST,nullptr);
        auto single=texture(kTaaFadeMaskFormat,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST,nullptr);
        auto bytes=buffer(uh,uint64_t(n)*256*3,D3D12_RESOURCE_STATE_GENERIC_READ);
        {uint8_t* p{};Check(bytes->Map(0,nullptr,reinterpret_cast<void**>(&p)),"mask map");
            for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x){p[size_t(y)*256+x]=x>=8 && x<24 && y>=8 && y<24 && !((x+y)&1u)?0:255;p[size_t(n+y)*256+x]=255;p[size_t(2*n+y)*256+x]=x==16 && y==16?0:255;}
            bytes->Unmap(0,nullptr);}
        for(uint32_t m=0;m<3;++m){D3D12_TEXTURE_COPY_LOCATION from{},to{};from.pResource=bytes.Get();from.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            from.PlacedFootprint.Offset=uint64_t(m)*n*256;from.PlacedFootprint.Footprint={kTaaFadeMaskFormat,n,n,1,256};to.pResource=m==2?single.Get():m?unmarked.Get():mask.Get();
            to.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&to,0,0,0,&from,nullptr);Barrier(list.Get(),to.pResource,D3D12_RESOURCE_STATE_COPY_DEST,psr|npsr);}
        submit();
        const auto j=ds3native::upscaler::Jitter(3,8);const size_t dot=16*n+16,hole=17*n+16;
        (void)frame(20,still.Get(),true,0);const auto reactive=frame(19,still.Get(),false,1,j[0],j[1],colours[18].Get());
        (void)frame(20,still.Get(),true,2);const auto faded=frame(19,still.Get(),false,0,j[0],j[1],colours[18].Get(),false,mask.Get());
        std::printf("T14: the fade mask: a dot %.3f (%.3f without it), a hole %.4f (%.3f)\n",faded[dot],reactive[dot],faded[hole],reactive[hole]);
        Require(std::fabs(faded[dot]-1.0f)<1e-3f && reactive[dot]<0.95f,"T14 a marked dot is its own sample");
        Require(faded[hole]<1e-3f && reactive[hole]>0.01f,"T14 a hole beside a marked dot keeps no history");
        (void)frame(20,still.Get(),true,1);const auto none=frame(19,still.Get(),false,2,j[0],j[1],colours[18].Get(),false,unmarked.Get());
        float worst=0;for(size_t i=0;i<none.size();++i)worst=(std::max)(worst,std::fabs(none[i]-reactive[i]));
        std::printf("T14: an unmarked mask against the base alone: %.5f at most\n",worst);
        Require(worst==0.0f,"T14 an unmarked mask leaves the frame as the base alone");
        (void)frame(1,still.Get(),true,0);const auto ring_plain=frame(0,still.Get(),false,1,j[0],j[1],colours[0].Get());
        (void)frame(1,still.Get(),true,2);const auto ring=frame(0,still.Get(),false,0,j[0],j[1],colours[0].Get(),false,single.Get());
        const size_t one=16*n+17,two=16*n+18;
        std::printf("T14: one marked pixel: one away %.4f (%.4f without it), two away %.4f (%.4f)\n",ring[one],ring_plain[one],ring[two],ring_plain[two]);
        Require(std::fabs(ring[one]-1.0f)<1e-3f && ring_plain[one]<0.95f,"T14 one pixel from a marked one is its own sample");
        Require(ring[two]==ring_plain[two] && ring_plain[two]>0.05f,"T14 two pixels from a marked one as without the mask");
        // A mask without the base is refused before any command (its step review's L4).
        {bool refused=false;const auto frames=taa.Frames(),resets=taa.Resets();
            Require(taa.Prepare(colours[19].Get(),depth.Get(),still.Get(),output.Get(),reason),reason.c_str());
            TaaParams p{};p.exposure=1.0f;p.fade_mask=mask.Get();try{taa.Record(list.Get(),colours[19].Get(),depth.Get(),still.Get(),output.Get(),p,0);}
            catch(const std::logic_error& e){refused=std::string(e.what()).find("without the base")!=std::string::npos;}
            submit();Require(refused,"T14 a fade mask without the base refused");
            Require(taa.Frames()==frames && taa.Resets()==resets,"T14 the refusal counts no frame");
            // The stage usable after it (its second and third step reviews' L3, L1, L2): a reset frame of the uniform grey, then a still
            // frame that resolves from that history (the refusal left it), the whole picture the grey.
            (void)frame(2,still.Get(),true,0);const auto after=frame(2,still.Get(),false,1);float lo{},hi{};spread(after,lo,hi);
            Require(taa.Frames()==frames+2 && taa.Resets()==resets+1 && std::fabs(lo-0.4f)<0.005f && std::fabs(hi-0.4f)<0.005f,"T14 the stage usable after the refusal");}
        Require(taa.ReactiveFrames()==10,"T14 the reactive frames counted");}
    // 15. I-0140 tranche 41 (D-0306): own_reset and the kernel. An unjittered reset frame with own_reset is the colour as drawn (the
    //     checkerboard: exactly 0 and 1, where the reconstruction keeps 0.10 of each side); a jittered reset ignores it (phase 6's
    //     reconstruction, colour 15), so does a frame that is not a reset (the same as without it); a jittered reset at kernel 8 is the
    //     reconstruction mirrored at 8, its edge steeper than at 2.29.
    {const auto own=frame(0,still.Get(),true,0,0.0f,0.0f,nullptr,false,nullptr,2.29f,true);float worst=0;
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x)worst=(std::max)(worst,std::fabs(own[size_t(y)*n+x]-texel(0,x,y)[0]));
        std::printf("T15: an unjittered reset with own_reset within %.4f of the drawn colour (the reconstruction's %.3f at (16, 16))\n",worst,current(0,16,16));
        Require(worst<1e-3f && std::fabs(current(0,16,16)-texel(0,16,16)[0])>0.05f,"T15 an unjittered reset with own_reset is the colour as drawn");
        const auto j=ds3native::upscaler::Jitter(6,8);const auto jittered=frame(15,still.Get(),true,1,j[0],j[1],nullptr,false,nullptr,2.29f,true);worst=0;
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x)worst=(std::max)(worst,std::fabs(jittered[size_t(y)*n+x]-current_j(15,int(x),int(y),j[0],j[1])));
        Require(worst<0.004f,"T15 a jittered reset ignores own_reset");
        (void)frame(1,still.Get(),true,2);const auto plain=frame(0,still.Get(),false,0);
        (void)frame(1,still.Get(),true,1);const auto owned=frame(0,still.Get(),false,2,0.0f,0.0f,nullptr,false,nullptr,2.29f,true);worst=0;
        for(size_t i=0;i<plain.size();++i)worst=(std::max)(worst,std::fabs(plain[i]-owned[i]));
        Require(worst<1e-4f,"T15 a frame that is not a reset ignores own_reset");
        const auto tight=frame(15,still.Get(),true,0,j[0],j[1],nullptr,false,nullptr,8.0f);worst=0;
        for(uint32_t y=0;y<n;++y)for(uint32_t x=0;x<n;++x)worst=(std::max)(worst,std::fabs(tight[size_t(y)*n+x]-current_j(15,int(x),int(y),j[0],j[1],8.0)));
        const size_t a=16*n+16,b=16*n+17;   // the pixel the edge crosses and the next one
        std::printf("T15: kernel 8 within %.4f of its reconstruction; the edge %.3f / %.3f (%.3f / %.3f at 2.29)\n",worst,tight[a],tight[b],jittered[a],jittered[b]);
        Require(worst<0.004f && tight[a]-tight[b]>jittered[a]-jittered[b]+0.05f,"T15 the kernel's reconstruction, steeper at 8");}
    // No debug-layer message at the warning level or worse.
    uint64_t messages{};for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T size{};info->GetMessage(i,nullptr,&size);std::vector<uint8_t> bytes(size);
        auto* m=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());if(SUCCEEDED(info->GetMessage(i,m,&size)) && m->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::printf("%s\n",m->pDescription);}}
    Require(!messages,"debug-layer messages");
    taa.Close();
    std::printf("PASS taa: a uniform scene kept, a flipping checkerboard resolved to a steady grey, a reset and an off-screen reprojection the current colour, the sharpening steepening an edge within its range, an infinite sample bounded, a wrong input refused, the motion's sign, the clip's colour, a jittered edge converging, a dissolve's hole taking the current colour (the reactive weight over the 3x3) and its view, a fading row's dots and holes their own samples under the fade mask, an unjittered reset its own samples and a tighter kernel, no debug-layer message\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL taa: %s\n",e.what());return 1;}}
