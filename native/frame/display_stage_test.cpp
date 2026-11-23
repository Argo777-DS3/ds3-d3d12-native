// W3 G0-3 host controls T4-T7 (GAMMA-COLOUR-PATH-READY 5.1): the display stage with the original's two shaders (read
// from the executable at their file offsets; the build checks their SHA-256), on WARP or hardware with the debug layer:
// a ramp in the intermediate drawn through the curve equals the CPU model of the pass (identity and both brightness
// ends), a resize keeps the pipeline and the curve, the upload happens only on a change, no debug message.
#include "../renderer/display_stage.h"
#include "display_blobs.h"
#include <dxgi1_6.h>
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
std::vector<uint8_t> FileBytes(const char* path,uint64_t offset,uint32_t bytes){
    std::ifstream file(path,std::ios::binary);Require(bool(file),"executable unreadable");file.seekg(std::streamoff(offset));
    std::vector<uint8_t> out(bytes);file.read(reinterpret_cast<char*>(out.data()),bytes);Require(file.gcount()==std::streamsize(bytes),"blob read");
    uint32_t total{};std::memcpy(&total,out.data()+24,4);Require(!std::memcmp(out.data(),"DXBC",4) && total==bytes,"DXBC header");return out;}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};return barrier;}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,D3D12_HEAP_TYPE type,UINT64 size,D3D12_RESOURCE_STATES state){
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=size;desc.Height=1;desc.DepthOrArraySize=desc.MipLevels=1;
    desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    ComPtr<ID3D12Resource> result;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"control buffer");return result;}
DisplayCurve CurveOf(float brightness){const float dst=2.2f+.5f*brightness;auto bytes=std::make_shared<std::vector<uint8_t>>(DisplayCurveBytes);
    Require(GenerateDisplayCurve({2.2f,2.2f,2.2f,dst,dst,dst},std::span<uint8_t,DisplayCurveBytes>(bytes->data(),DisplayCurveBytes)),"control curve");return bytes;}
struct Rig {
    ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
    ComPtr<ID3D12Fence> fence;uint64_t value{};HANDLE event{};ComPtr<ID3D12Resource> target,ramp,readback;ComPtr<ID3D12DescriptorHeap> rtv;
    static constexpr UINT size=16;
    explicit Rig(IDXGIAdapter1* adapter){
        Check(D3D12CreateDevice(adapter,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"control device");
        const D3D12_COMMAND_QUEUE_DESC queue_desc{D3D12_COMMAND_LIST_TYPE_DIRECT};Check(device->CreateCommandQueue(&queue_desc,IID_PPV_ARGS(&queue)),"control queue");
        Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"control allocator");
        Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"control list");list->Close();
        Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"control fence");event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=desc.Height=size;desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_CLEAR_VALUE clear{DXGI_FORMAT_R8G8B8A8_UNORM,{0,0,0,1}};
        Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_RENDER_TARGET,&clear,IID_PPV_ARGS(&target)),"control back buffer");
        D3D12_DESCRIPTOR_HEAP_DESC heaps{D3D12_DESCRIPTOR_HEAP_TYPE_RTV,1};Check(device->CreateDescriptorHeap(&heaps,IID_PPV_ARGS(&rtv)),"control rtv heap");
        device->CreateRenderTargetView(target.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
        // The ramp: pixel (x, y) channel c = (16 y + x + 85 c) mod 256, so the 256 values appear in every channel.
        ramp=Buffer(device.Get(),D3D12_HEAP_TYPE_UPLOAD,256*size,D3D12_RESOURCE_STATE_GENERIC_READ);
        uint8_t* memory{};const D3D12_RANGE none{};Check(ramp->Map(0,&none,reinterpret_cast<void**>(&memory)),"control ramp map");
        for(UINT y=0;y<size;++y)for(UINT x=0;x<size;++x)for(UINT c=0;c<4;++c)memory[y*256+x*4+c]=c==3?255:uint8_t((16*y+x+85*c)%256);
        ramp->Unmap(0,nullptr);readback=Buffer(device.Get(),D3D12_HEAP_TYPE_READBACK,256*size,D3D12_RESOURCE_STATE_COPY_DEST);}
    ~Rig(){if(event)CloseHandle(event);}
    void Begin(){Check(allocator->Reset(),"control allocator reset");Check(list->Reset(allocator.Get(),nullptr),"control list reset");}
    void Finish(){Check(list->Close(),"control list close");ID3D12CommandList* lists[]{list.Get()};queue->ExecuteCommandLists(1,lists);
        Check(queue->Signal(fence.Get(),++value),"control signal");Check(fence->SetEventOnCompletion(value,event),"control wait");WaitForSingleObject(event,INFINITE);}
    // The ramp into the intermediate, the pass into the target, the target read back (RGBA8 rows of 256 bytes).
    std::vector<uint8_t> Run(DisplayStage& stage){
        Begin();auto* intermediate=stage.Intermediate();
        auto barrier=Transition(intermediate,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_DEST);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION to{intermediate,D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX},from{ramp.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT};
        from.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,size,size,1,256};list->CopyTextureRegion(&to,0,0,0,&from,nullptr);
        barrier=Transition(intermediate,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);list->ResourceBarrier(1,&barrier);
        const auto staging=stage.RecordUploadIfDirty(list.Get());
        stage.RecordPass(list.Get(),rtv->GetCPUDescriptorHandleForHeapStart(),{0,0,float(size),float(size),0,1},{0,0,LONG(size),LONG(size)});
        barrier=Transition(intermediate,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);list->ResourceBarrier(1,&barrier);
        barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE);list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION out{readback.Get(),D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT},source{target.Get(),D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
        out.PlacedFootprint.Footprint={DXGI_FORMAT_R8G8B8A8_UNORM,size,size,1,256};list->CopyTextureRegion(&out,0,0,0,&source,nullptr);
        barrier=Transition(target.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_RENDER_TARGET);list->ResourceBarrier(1,&barrier);Finish();
        std::vector<uint8_t> pixels(256*size);void* memory{};const D3D12_RANGE read{0,pixels.size()};Check(readback->Map(0,&read,&memory),"control readback");
        std::memcpy(pixels.data(),memory,pixels.size());const D3D12_RANGE none{};readback->Unmap(0,&none);return pixels;}
    uint64_t DebugMessages() const {
        ComPtr<ID3D12InfoQueue> info;if(FAILED(device.As(&info)))return 0;uint64_t found{};
        for(UINT64 index=0;index<info->GetNumStoredMessages();++index){SIZE_T size_bytes{};info->GetMessage(index,nullptr,&size_bytes);std::vector<uint8_t> bytes(size_bytes);
            auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
            if(SUCCEEDED(info->GetMessage(index,message,&size_bytes)) && message->Severity<=D3D12_MESSAGE_SEVERITY_WARNING){++found;std::fprintf(stderr,"%s\n",message->pDescription);}}
        return found;}
};
// Every pixel of the read-back ramp against the CPU model of the pass with the given curve (within `tolerance`).
void Compare(const std::vector<uint8_t>& pixels,const DisplayCurve& curve,int tolerance,const char* what){
    std::array<std::array<float,DisplayCurveEntries>,3> slices{};DeinterleaveDisplayCurve(std::span<const uint8_t,DisplayCurveBytes>(curve->data(),DisplayCurveBytes),slices);
    for(UINT y=0;y<Rig::size;++y)for(UINT x=0;x<Rig::size;++x)for(UINT c=0;c<3;++c){const uint8_t in=uint8_t((16*y+x+85*c)%256);
        if(std::abs(int(pixels[y*256+x*4+c])-int(DisplayPassModel(slices[c],in)))>tolerance)throw std::runtime_error(what);}
}
}
int main(int argc,char** argv){try{
    Require(argc==3,"usage: display_stage_test warp|hardware DarkSoulsIII.exe");const bool warp=!std::strcmp(argv[1],"warp");
    Require(warp || !std::strcmp(argv[1],"hardware"),"unknown profile");
    DisplayBlobs blobs{FileBytes(argv[2],bootstrap::display_vertex_blob.file_offset,bootstrap::display_vertex_blob.bytes),
        FileBytes(argv[2],bootstrap::display_pixel_blob.file_offset,bootstrap::display_pixel_blob.bytes)};
    ComPtr<ID3D12Debug> debug;if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))debug->EnableDebugLayer();
    ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&factory)),"control factory");
    ComPtr<IDXGIAdapter1> adapter;
    if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"control WARP adapter");
    else{Check(factory->EnumAdapterByGpuPreference(0,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter)),"control hardware adapter");
        DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);Require(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE),"no hardware adapter");}
    Rig rig(adapter.Get());DisplayStage stage;std::string reason;
    Require(stage.Open(rig.device.Get(),blobs,Rig::size,Rig::size,reason),reason.c_str());
    // T4: the identity curve (opened with it) maps every value to itself; the two brightness ends follow the model.
    const auto identity=CurveOf(0);const auto first=rig.Run(stage);Compare(first,identity,0,"T4 identity pass not exact");
    for(UINT y=0;y<Rig::size;++y)for(UINT x=0;x<Rig::size;++x)for(UINT c=0;c<3;++c)Require(first[y*256+x*4+c]==uint8_t((16*y+x+85*c)%256),"T4 identity changes a value");
    Require(stage.Uploads()==1,"T6 the identity curve uploaded once at the first frame");
    stage.SetCurve(identity);rig.Run(stage);Require(stage.Uploads()==1,"T6 an equal curve uploaded again");
    for(const float b:{-1.f,1.f}){const auto curve=CurveOf(b);stage.SetCurve(curve);Compare(rig.Run(stage),curve,1,"T4 brightness end differs from the model");}
    Require(stage.Uploads()==3,"T6 each changed curve uploaded once");
    // T5: a resize keeps the pipeline and the curve (no upload) and draws the same image at the same brightness.
    auto* curve_texture=stage.Curve();stage.SetCurve(identity);const auto uploads=stage.Uploads();
    stage.Resize(Rig::size,Rig::size);Require(stage.Curve()==curve_texture,"T5 the curve recreated by a resize");
    Compare(rig.Run(stage),identity,0,"T5 image after the resize");Require(stage.Uploads()==uploads+1,"T5 the pending identity curve uploaded once");
    // T7: no debug message (warnings included, as the renderer counts them).
    Require(rig.DebugMessages()==0,"T7 debug-layer messages");
    std::printf("PASS display stage %s: T4 the original shaders draw the ramp through the identity curve unchanged and through both brightness ends as the CPU model (within 1), "
        "T5 a resize keeps the pipeline and the curve, T6 one upload per changed curve, T7 no debug-layer message\n",argv[1]);
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
