// I-0138 tranche 5 (ROADMAP workstream 11): the HDR10 output's numbers (renderer/hdr_output.h) and its stage (OutputStage).
//  1. The ST 2084 curve against published values of the standard (0, 100, 203, 1,000, 4,000 and 10,000 nits; connaissance
//     générale: 0.5081, 0.5807 (BT.2408's reference white), 0.7518, 0.9026), its inverse over the range, the Rec. 709 to
//     Rec. 2020 matrix (each row sums to 1), the peak's rule, the highlights' continuation and the output's roll-off (equal to
//     their argument below their start, continuous, their slope 1 there, rising, the roll-off under its top).
//  2. The C++ block's layout is the compiled programme's cbuffer (D3DReflect on the build's post_output_scene_ps; the copy's
//     programme reads no constant).
//  3. With a profile (warp or hardware), the stage's draws into an R10G10B10A2_UNORM target, read back and compared with
//     hdr::EncodedToHdr10, without the dither within one 10-bit code (half a code of rounding, the rest the card's pow, log
//     and exp against the model's doubles):
//     - RecordCopy then RecordScene: a 16x16 8-bit picture holding the 256 values in every channel, copied into the stage's
//       float picture and sent through the transform, at 80 and 200 nits of paper white and at a peak under the paper white
//       (the clamp);
//     - RecordScene: a float picture from 0 to 3 (encoded: up to 11 times the paper white, so the roll-off and the clamp run),
//       at peaks of 1,000 and 300 nits (I-0138 tranche 5b: the game's brightness option no longer reaches the HDR10 chain, so
//       its two cases and the check that it stopped at the paper white went with the parameter);
//     - with the dither: each code within one and a half of the model's, their mean error under a fifth of a code (a dither
//       of one step that does not move the mean).
//     Until the UI was drawn in place (the third step's witness) the stage had an 8-bit picture's own programme and a UI layer
//     blended by the scene's programme, each with its case here; both are gone with their cases.
#include "../renderer/output_stage.h"
#include "../renderer/backend.h"
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include "post_output_scene_ps.dxbc.h"

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
int failures{},checks{};
void True(const char* what,bool value){++checks;if(value)return;++failures;std::printf("FAIL %s\n",what);}
void Near(const char* what,double got,double expected,double tolerance){++checks;if(std::abs(got-expected)<=tolerance)return;++failures;std::printf("FAIL %s: %.7f, expected %.7f\n",what,got,expected);}
void Check(HRESULT status,const char* what){if(FAILED(status))throw std::runtime_error(what);}
void Numbers(){
    Near("PQ(0)",hdr::PqEncode(0),0.0,1e-6);Near("PQ(10000)",hdr::PqEncode(10000),1.0,1e-12);
    Near("PQ(100)",hdr::PqEncode(100),0.5081,5e-5);Near("PQ(203)",hdr::PqEncode(203),0.5807,5e-5);
    Near("PQ(1000)",hdr::PqEncode(1000),0.7518,5e-5);Near("PQ(4000)",hdr::PqEncode(4000),0.9026,5e-5);
    for(const double nits:{0.005,0.1,1.0,80.0,200.0,1000.0,9999.0})Near("PQ round trip",hdr::PqDecode(hdr::PqEncode(nits)),nits,nits*1e-9);
    True("PQ rises",hdr::PqEncode(80)<hdr::PqEncode(81) && hdr::PqEncode(-5)==hdr::PqEncode(0) && hdr::PqEncode(20000)==1.0);
    for(int r=0;r<3;++r)Near("Rec. 2020 row sum",double(hdr::kRec709ToRec2020[r][0])+hdr::kRec709ToRec2020[r][1]+hdr::kRec709ToRec2020[r][2],1.0,1e-4);
    hdr::Request request;hdr::Display display;
    True("peak: 1,000 nits without a screen's maximum",hdr::Peak(request,display)==1000.0f);
    display.max_nits=1499.0f;True("peak: the screen's maximum",hdr::Peak(request,display)==1499.0f);
    display.max_nits=150.0f;True("peak: a screen's maximum under the paper white is not taken",hdr::Peak(request,display)==1000.0f);
    request.peak=600.0f;True("peak: the asked one",hdr::Peak(request,display)==600.0f);
    request.peak=50.0f;True("peak: never under the paper white",hdr::Peak(request,display)==200.0f);
    const auto p=hdr::BuildOutputParams(200.0f,1000.0f);const float white[3]{1,1,1},black[3]{0,0,0};double codes[3]{};
    hdr::EncodedToHdr10(white,p,codes);for(const double code:codes)Near("an SDR white is the paper white",code,1023.0*hdr::PqEncode(200),0.06);   // the matrix's four decimals
    hdr::EncodedToHdr10(black,p,codes);for(const double code:codes)Near("an SDR black is 0",code,0.0,1e-3);
    // The highlights (the tone map's HDR entry) and the roll-off (the output's).
    for(const double knee:{0.5,0.75,0.9}){
        True("highlight: the value itself up to the knee",hdr::Highlight(0.0,knee)==0.0 && hdr::Highlight(knee*0.5,knee)==knee*0.5 && hdr::Highlight(knee,knee)==knee);
        Near("highlight: continuous at the knee",hdr::Highlight(knee+1e-9,knee),knee,1e-8);
        Near("highlight: the slope 1 at the knee",(hdr::Highlight(knee+1e-5,knee)-knee)/1e-5,1.0,1e-3);
        bool rises=true;double last=knee;for(int i=1;i<=1000;++i){const double v=knee+(1.0-knee)*i/1000.0,h=hdr::Highlight(v,knee);rises=rises && h>=last && h>=v;last=h;}
        True("highlight: rises, never under the value, finite at 1",rises && std::isfinite(hdr::Highlight(1.0,knee)));}
    Near("highlight: knee 0.75, 0.99",hdr::Highlight(0.99,0.75),0.75+0.25*std::log(25.0),1e-12);
    for(const double top:{1.5,5.0,20.0}){const double start=(std::max)(1.0,0.5*top);
        True("roll-off: the value itself up to its start",hdr::RollOff(0.3,top)==0.3 && hdr::RollOff(start,top)==start);
        Near("roll-off: the slope 1 at its start",(hdr::RollOff(start+1e-6,top)-start)/1e-6,1.0,1e-3);
        bool under=true;double last=start;for(int i=1;i<=1000;++i){const double m=start+i*0.05,r=hdr::RollOff(m,top);under=under && r>=last && r<=top && r<=m;last=r;}
        True("roll-off: rises, under its top and under the value",under);}
    True("roll-off: none when the top is the paper white or under it",hdr::RollOff(3.0,1.0)==3.0 && hdr::RollOff(3.0,0.6)==3.0);   // the clamp to the peak then
}
void Layout(const char* programme,const void* code,size_t bytes){
    ComPtr<ID3D12ShaderReflection> reflection;
    if(FAILED(D3DReflect(code,bytes,IID_PPV_ARGS(&reflection)))){True((std::string(programme)+": reflection").c_str(),false);return;}
    auto* block=reflection->GetConstantBufferByName("OutputParams");D3D12_SHADER_BUFFER_DESC desc{};
    if(FAILED(block->GetDesc(&desc))){True((std::string(programme)+": no OutputParams cbuffer").c_str(),false);return;}
    True((std::string(programme)+": the cbuffer's size").c_str(),desc.Size==sizeof(hdr::OutputParams));
    struct Field {const char* name;size_t offset,size;};
    const Field fields[]{{"rec2020",offsetof(hdr::OutputParams,rec2020),48},{"paper_white",offsetof(hdr::OutputParams,paper_white),4},
        {"peak",offsetof(hdr::OutputParams,peak),4},{"sdr_gamma",offsetof(hdr::OutputParams,sdr_gamma),4},{"flags",offsetof(hdr::OutputParams,flags),4},
        {"noise_offset",offsetof(hdr::OutputParams,noise_offset),8},{"pad",offsetof(hdr::OutputParams,pad),8}};
    for(const auto& f:fields){D3D12_SHADER_VARIABLE_DESC v{};
        const bool ok=SUCCEEDED(block->GetVariableByName(f.name)->GetDesc(&v)) && v.StartOffset==f.offset && v.Size==f.size;
        True((std::string(programme)+": the cbuffer's "+f.name).c_str(),ok);}
}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;}
ComPtr<ID3D12Resource> Texture(ID3D12Device* device,DXGI_FORMAT format,UINT size,D3D12_RESOURCE_FLAGS flags,D3D12_RESOURCE_STATES state,const float* clear=nullptr){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=desc.Height=size;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Format=format;desc.Flags=flags;D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_CLEAR_VALUE value{};value.Format=format;if(clear)for(int c=0;c<4;++c)value.Color[c]=clear[c];
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,clear?&value:nullptr,IID_PPV_ARGS(&result)),"control texture");return result;}
// Half floats (round to nearest even) and back: the scene's texels as the card holds them.
uint16_t Half(float f){
    uint32_t x;std::memcpy(&x,&f,4);const uint32_t sign=(x>>16)&0x8000u;x&=0x7FFFFFFFu;
    if(x>=0x477FF000u)return uint16_t(sign|0x7C00u);
    if(x<0x38800000u){if(x<0x33000000u)return uint16_t(sign);
        const uint32_t m=(x&0x7FFFFFu)|0x800000u,shift=126u-(x>>23);uint32_t r=m>>shift;const uint32_t rest=m&((1u<<shift)-1u),half=1u<<(shift-1u);
        if(rest>half || (rest==half && (r&1u)))++r;return uint16_t(sign|r);}
    uint32_t h=(((x>>23)-112u)<<10)|((x>>13)&0x3FFu);const uint32_t rest=x&0x1FFFu;if(rest>0x1000u || (rest==0x1000u && (h&1u)))++h;return uint16_t(sign|h);}
float FromHalf(uint16_t h){
    const uint32_t exponent=(h>>10)&31u,mantissa=h&0x3FFu;const float sign=(h&0x8000u)?-1.0f:1.0f;
    if(!exponent)return sign*std::ldexp(float(mantissa),-24);
    return sign*std::ldexp(float(mantissa|0x400u),int(exponent)-25);}
// The stage's draws on a device.
void Stage(bool warp){
    constexpr UINT size=16;
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"control factory");
    ComPtr<IDXGIAdapter1> adapter;
    if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"control WARP adapter");
    else{Check(factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter)),"control hardware adapter");
        DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);if(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)throw std::runtime_error("no hardware adapter");}
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"control device");
    ComPtr<ID3D12CommandQueue> queue;const D3D12_COMMAND_QUEUE_DESC queue_desc{D3D12_COMMAND_LIST_TYPE_DIRECT};Check(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)),"control queue");
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"control allocator");
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");list->Close();
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"control fence");uint64_t value{};const HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    constexpr float sentinel[4]{1,0,1,0};   // the target's clear value: a draw that wrote nothing is seen
    const auto sdr=Texture(device.Get(),DXGI_FORMAT_R8G8B8A8_UNORM,size,D3D12_RESOURCE_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto target=Texture(device.Get(),OutputStage::Format(),size,D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,D3D12_RESOURCE_STATE_RENDER_TARGET,sentinel);
    const auto ramp=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,256*size,D3D12_RESOURCE_STATE_GENERIC_READ);
    const auto scene_upload=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,256*size,D3D12_RESOURCE_STATE_GENERIC_READ);
    const auto readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,256*size,D3D12_RESOURCE_STATE_COPY_DEST);
    const auto constants=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,256,D3D12_RESOURCE_STATE_GENERIC_READ);
    // The pictures. SDR: pixel (x, y) channel c = (16 y + x + 85 c) mod 256 (display_stage_test's ramp). Float: channel c of
    // texel i = 16 y + x holds 3 ((i + 85 c) mod 256) / 255, as a half float.
    float scene_values[256][3]{};
    {uint8_t* memory{};const D3D12_RANGE none{};Check(ramp->Map(0,&none,reinterpret_cast<void**>(&memory)),"control ramp map");
        for(UINT y=0;y<size;++y)for(UINT x=0;x<size;++x)for(UINT c=0;c<4;++c)memory[y*256+x*4+c]=c==3?255:uint8_t((16*y+x+85*c)%256);
        ramp->Unmap(0,nullptr);
        Check(scene_upload->Map(0,&none,reinterpret_cast<void**>(&memory)),"control scene map");
        for(UINT i=0;i<256;++i){uint16_t texel[4]{};for(UINT c=0;c<3;++c){texel[c]=Half(3.0f*float((i+85*c)%256)/255.0f);scene_values[i][c]=FromHalf(texel[c]);}
            texel[3]=Half(1.0f);std::memcpy(memory+(i/16)*256+(i%16)*8,texel,8);}
        scene_upload->Unmap(0,nullptr);}
    void* block{};{const D3D12_RANGE none{};Check(constants->Map(0,&none,&block),"control constants map");}
    ComPtr<ID3D12DescriptorHeap> rtv;const D3D12_DESCRIPTOR_HEAP_DESC heaps{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};Check(device->CreateDescriptorHeap(&heaps,IID_PPV_ARGS(&rtv)),"control rtv heap");
    device->CreateRenderTargetView(target.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    OutputStage stage;std::string reason;if(!stage.Open(device.Get(),reason))throw std::runtime_error("output stage: "+reason);
    if(!stage.Targets(size,size,reason))throw std::runtime_error("output stage targets: "+reason);
    const auto begin=[&]{Check(allocator->Reset(),"control allocator reset");Check(list->Reset(allocator.Get(),nullptr),"control list reset");};
    const auto finish=[&]{Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        Check(queue->Signal(fence.Get(),++value),"control signal");Check(fence->SetEventOnCompletion(value,event),"control wait");WaitForSingleObject(event,INFINITE);};
    constexpr auto rt=D3D12_RESOURCE_STATE_RENDER_TARGET,psr=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,dest=D3D12_RESOURCE_STATE_COPY_DEST;
    // The 8-bit picture uploaded once; the stage's picture is a render target between Records, as in the renderer.
    begin();
    {D3D12_TEXTURE_COPY_LOCATION to{sdr.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{ramp.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,size,size,1,256};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        const auto barrier=Transition(sdr.Get(),dest,psr);list->ResourceBarrier(1,&barrier);}
    finish();
    const D3D12_VIEWPORT viewport{0,0,float(size),float(size),0,1};const D3D12_RECT scissor{0,0,LONG(size),LONG(size)};
    const auto sink=[&](uint64_t){return OutputConstantMemory{block,constants->GetGPUVirtualAddress()};};
    // One draw read back: the 256 texels' codes.
    const auto read=[&](const std::function<void()>& record){
        begin();list->ClearRenderTargetView(rtv->GetCPUDescriptorHandleForHeapStart(),sentinel,0,nullptr);record();
        auto barrier=Transition(target.Get(),rt,D3D12_RESOURCE_STATE_COPY_SOURCE);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION out{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},source{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        out.PlacedFootprint.Footprint={OutputStage::Format(),size,size,1,256};list->CopyTextureRegion(&out,0,0,0,&source,nullptr);
        barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,rt);list->ResourceBarrier(1,&barrier);finish();
        std::vector<uint32_t> texels(256);void* memory{};const D3D12_RANGE range{0,256*size};Check(readback->Map(0,&range,&memory),"control readback");
        for(UINT i=0;i<256;++i)std::memcpy(&texels[i],static_cast<const uint8_t*>(memory)+(i/16)*256+(i%16)*4,4);const D3D12_RANGE none{};readback->Unmap(0,&none);return texels;};
    double worst{};uint64_t draws{};
    // texel(i): the display-encoded value the programme reads for texel i. Returns the worst difference and the mean error, in codes.
    const auto compare=[&](const std::vector<uint32_t>& texels,const hdr::OutputParams& params,const std::function<void(UINT,float[3])>& texel,double& mean){
        double difference{},sum{};bool alpha=true;
        for(UINT i=0;i<256;++i){float in[3]{};texel(i,in);double expected[3]{};hdr::EncodedToHdr10(in,params,expected);
            for(UINT c=0;c<3;++c){const double d=double((texels[i]>>(10*c))&1023u)-expected[c];difference=(std::max)(difference,std::abs(d));sum+=d;}
            alpha=alpha && (texels[i]>>30)==3u;}
        True("the stage's alpha is 1",alpha);mean=sum/768.0;++draws;return difference;};
    const auto sdr_texel=[](UINT i,float in[3]){for(UINT c=0;c<3;++c)in[c]=float((i+85*c)%256)/255.0f;};
    const auto scene_texel=[&](UINT i,float in[3]){for(UINT c=0;c<3;++c)in[c]=scene_values[i][c];};
    // The transform of the stage's picture, its state taken to a source and back.
    const auto transform=[&](const hdr::OutputParams& params){
        auto barrier=Transition(stage.Scene(),rt,psr);list->ResourceBarrier(1,&barrier);
        stage.RecordScene(list.Get(),rtv->GetCPUDescriptorHandleForHeapStart(),params,sink,viewport,scissor);
        barrier=Transition(stage.Scene(),psr,rt);list->ResourceBarrier(1,&barrier);};
    struct Case {float paper,peak;const char* name;};
    for(const Case& each:{Case{80.0f,1000.0f,"80 nits"},Case{200.0f,1000.0f,"200 nits"},Case{200.0f,120.0f,"a peak under the white"}}){
        const auto params=hdr::BuildOutputParams(each.paper,each.peak);double mean{};
        const auto texels=read([&]{stage.RecordCopy(list.Get(),sdr.Get(),1,viewport,scissor);transform(params);});
        const double difference=compare(texels,params,sdr_texel,mean);worst=(std::max)(worst,difference);
        True((std::string("the 8-bit picture, copied, within one code of the model: ")+each.name).c_str(),difference<=1.0);
        if(difference>1.0)std::printf("  worst difference %.3f codes\n",difference);}
    // The float picture's own values in place of the copy's.
    begin();
    {D3D12_RESOURCE_BARRIER barrier=Transition(stage.Scene(),rt,dest);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION to{stage.Scene(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{scene_upload.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint.Footprint={OutputStage::SceneFormat(),size,size,1,256};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        barrier=Transition(stage.Scene(),dest,rt);list->ResourceBarrier(1,&barrier);}
    finish();
    for(const Case& each:{Case{200.0f,1000.0f,"a peak of 1,000 nits"},Case{200.0f,300.0f,"a peak of 300 nits"}}){
        auto params=hdr::BuildOutputParams(each.paper,each.peak);double mean{};
        const auto texels=read([&]{transform(params);});
        const double difference=compare(texels,params,scene_texel,mean);worst=(std::max)(worst,difference);
        True((std::string("the float picture's codes within one of the model's: ")+each.name).c_str(),difference<=1.0);
        if(difference>1.0)std::printf("  worst difference %.3f codes\n",difference);
        // The dither: one step of blue noise, at two places of the texture.
        for(const int offset:{0,37}){auto dithered=params;dithered.flags=hdr::kOutputDither;dithered.noise_offset[0]=offset;dithered.noise_offset[1]=offset/2;
            const auto noisy=read([&]{transform(dithered);});
            const double spread=compare(noisy,dithered,scene_texel,mean);
            True((std::string("the dithered codes within one and a half of the model's: ")+each.name).c_str(),spread<=1.5);
            True((std::string("the dither does not move the mean: ")+each.name).c_str(),std::abs(mean)<0.2);
            True("the dither changes some codes",noisy!=texels);
            if(spread>1.5 || std::abs(mean)>=0.2)std::printf("  dither: worst %.3f codes, mean %.3f\n",spread,mean);}}
    True("the stage counted its draws",stage.ScenePasses()==draws && stage.CopyPasses()==3);
    ComPtr<ID3D12InfoQueue> info;uint64_t messages{};
    True("the debug layer's message queue is there (no message is a result only with it)",SUCCEEDED(device.As(&info)));
    if(info)for(UINT64 i=0;i<info->GetNumStoredMessages();++i){SIZE_T bytes{};info->GetMessage(i,nullptr,&bytes);std::vector<uint8_t> storage(bytes);
        auto* message=reinterpret_cast<D3D12_MESSAGE*>(storage.data());
        if(SUCCEEDED(info->GetMessage(i,message,&bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++messages;std::printf("  %s\n",message->pDescription);}}
    True("no debug-layer message",messages==0);
    stage.Close();CloseHandle(event);
    std::printf("  stage %s: worst difference without the dither %.3f codes of 1023 over %llu draws of 256 texels\n",warp?"warp":"hardware",worst,static_cast<unsigned long long>(draws));
}
}
int main(int argc,char** argv){
    try{
        Numbers();Layout("post_output_scene_ps",post_output_scene_ps_dxbc,sizeof post_output_scene_ps_dxbc);
        if(argc>=2){const bool warp=!std::strcmp(argv[1],"warp");if(!warp && std::strcmp(argv[1],"hardware"))throw std::runtime_error("unknown profile");Stage(warp);}
    }catch(const std::exception& error){std::printf("FAIL hdr output: %s\n",error.what());return 1;}
    if(failures){std::printf("FAIL hdr output: %d of %d checks\n",failures,checks);return 1;}
    std::printf("PASS hdr output%s%s: %d checks\n",argc>=2?" ":"",argc>=2?argv[1]:"",checks);return 0;
}
