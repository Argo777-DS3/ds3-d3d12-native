#include "game_model_selection.h"
#include "../renderer/model_geometry.h"
#include "../renderer/backend.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>

using namespace ds3native;
using Microsoft::WRL::ComPtr;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<size_t N,class T>void Set(std::array<uint8_t,N>& bytes,size_t at,T value){
    Require(at<=N-sizeof(T),"test extent");std::memcpy(bytes.data()+at,&value,sizeof(value));
}
template<class F>void Refuses(F action){bool refused{};try{action();}catch(const std::invalid_argument&){refused=true;}
    Require(refused,"bad native binding accepted");}
ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,uint64_t size,D3D12_HEAP_TYPE type) {
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=size;d.Height=1;
    d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES h{};h.Type=type;h.CreationNodeMask=h.VisibleNodeMask=1;
    ComPtr<ID3D12Resource> value;
    Require(SUCCEEDED(device->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&d,
        type==D3D12_HEAP_TYPE_UPLOAD?D3D12_RESOURCE_STATE_GENERIC_READ:D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,IID_PPV_ARGS(&value))),"native buffer creation");return value;
}
void Fill(ID3D12Resource* resource,uint8_t value){void* bytes{};D3D12_RANGE empty{};
    Require(SUCCEEDED(resource->Map(0,&empty,&bytes)),"buffer map");
    std::memset(bytes,value,static_cast<size_t>(resource->GetDesc().Width));resource->Unmap(0,nullptr);}
void Inputs() {
    // Synthetic in-process CPU fields exercise the independently decompiled
    // association contract. They are not a DS3 runtime witness or game asset.
    constexpr uintptr_t image=0x140000000;
    std::array<uint8_t,0x208> model{};std::array<uint8_t,0xB08> entity{};
    std::array<uint8_t,0x58> source{};std::array<uint8_t,0x90> meshes{};
    std::array<uintptr_t,2> materials{17,29};std::array<uintptr_t,2> instance_range{};
    Set(model,0,image+ds3content::model_vtable_rva);Set(model,0xE8,uintptr_t(91));
    Set(model,0xA0,int32_t(2));Set(model,0xA8,reinterpret_cast<uintptr_t>(meshes.data()));
    Set(model,0x1F8,reinterpret_cast<uintptr_t>(materials.data()));
    Set(model,0x200,reinterpret_cast<uintptr_t>(materials.data()+materials.size()));
    Set(entity,0xA60,reinterpret_cast<uintptr_t>(source.data()));Set(source,0x50,uintptr_t(123));
    Set(meshes,0,uint16_t(1));Set(meshes,2,uint16_t(1));Set(meshes,8,uint32_t(4));
    Set(meshes,0x10,uint32_t(3));Set(meshes,0x14,uint16_t(1));Set(meshes,0x40,uintptr_t(321));
    Set(meshes,0x48,uint16_t(0));Set(meshes,0x48+2,uint16_t(0));Set(meshes,0x48+0x14,uint16_t(2));
    Set(meshes,0x48+0x40,uintptr_t(654));
    ingress::CurrentModelInputs selected;
    const auto read=[&]{return ingress::ReadCurrentModelInputs(image,reinterpret_cast<uintptr_t>(entity.data()),
        reinterpret_cast<uintptr_t>(model.data()),selected);};
    Require(read() && selected.meshes.size()==2,"current mesh read");
    Require(selected.meshes[0].material==29 && selected.meshes[1].material==17,"material selected by id");
    Require(selected.meshes[0].patch_index==1 && selected.meshes[0].ordinal==0,"patch index replaced by ordinal");
    Require(selected.meshes[0].layout==321 && selected.meshes[0].flags==3 && selected.meshes[1].lod_mask==2,"mesh policy lost");
    Require(selected.pose==reinterpret_cast<uintptr_t>(entity.data())+0xA80 && selected.patch==123,"current instance association");
    materials[1]=47;Set(source,0x50,uintptr_t(456));Require(read() && selected.meshes[0].material==47 && selected.patch==456,"stale current input");
    const auto retained=selected;Set(meshes,0,uint16_t(2));Require(!read(),"out-of-range material accepted");
    Require(selected.meshes[0].material==retained.meshes[0].material,"failed read published partial input");Set(meshes,0,uint16_t(1));
    instance_range={1024,1168};Set(entity,0xA48,reinterpret_cast<uintptr_t>(instance_range.data()));
    Require(read() && selected.instance_streams,"instance stream membership lost");
    instance_range[1]=1025;Require(!read(),"fractional instance stream accepted");
    Set(model,0xA0,int32_t(-1));Require(!read(),"negative mesh count accepted");
}
void Geometry(bool warp) {
    DeviceQueues gpu;gpu.Open(warp,true);
    auto authored=Buffer(gpu.device.Get(),256,D3D12_HEAP_TYPE_UPLOAD);Fill(authored.Get(),11);
    auto first=Buffer(gpu.device.Get(),96,D3D12_HEAP_TYPE_UPLOAD);Fill(first.Get(),37);
    auto second=Buffer(gpu.device.Get(),96,D3D12_HEAP_TYPE_UPLOAD);Fill(second.Get(),93);
    const MeshSlice slice{32,3,32,128,3,1};
    const auto static_draw=BindModelGeometry(slice,7,authored);
    const auto old_draw=BindModelGeometry(slice,7,authored,13,first);
    const auto new_draw=BindModelGeometry(slice,7,authored,14,second);
    Require(static_draw.vertices.BufferLocation==authored->GetGPUVirtualAddress()+32,"authored resource detached");
    Require(old_draw.vertices.BufferLocation==first->GetGPUVirtualAddress() && new_draw.vertices.BufferLocation==second->GetGPUVirtualAddress(),"stream version detached");
    Require(old_draw.indices.BufferLocation==authored->GetGPUVirtualAddress()+128 && old_draw.index_resource==new_draw.index_resource,"index storage changed by override");
    Refuses([&]{BindModelGeometry(slice,7,authored,0,first);});
    auto bad=slice;bad.index_offset=255;Refuses([&]{BindModelGeometry(bad,7,authored);});
    bad=slice;bad.vertex_count=4;Refuses([&]{BindModelGeometry(bad,7,authored,13,first);});
    // 1202: R32 face sets bind R32_UINT over four bytes per index; any other width is refused.
    auto wide=slice;wide.index_width=4;const auto wide_draw=BindModelGeometry(wide,7,authored);
    Require(wide_draw.indices.Format==DXGI_FORMAT_R32_UINT && wide_draw.indices.SizeInBytes==12 && static_draw.indices.Format==DXGI_FORMAT_R16_UINT &&
        static_draw.indices.SizeInBytes==6,"index width not carried to the view");
    wide.index_offset=248;Refuses([&]{BindModelGeometry(wide,7,authored);});
    bad=slice;bad.index_width=3;Refuses([&]{BindModelGeometry(bad,7,authored);});
    bad=wide;bad.index_offset=130;Refuses([&]{BindModelGeometry(bad,7,authored);});   // unaligned R32 view
    first.Reset();second.Reset();authored.Reset(); // producer replacement/withdrawal
    auto readback=Buffer(gpu.device.Get(),48,D3D12_HEAP_TYPE_READBACK);
    FrameRecorder recorder;recorder.Open(gpu.device.Get());
    Require(SUCCEEDED(recorder.allocator->Reset()) && SUCCEEDED(recorder.list->Reset(recorder.allocator.Get(),nullptr)),"copy list reset");
    recorder.list->CopyBufferRegion(readback.Get(),0,old_draw.vertex_resource.Get(),0,16);
    recorder.list->CopyBufferRegion(readback.Get(),16,new_draw.vertex_resource.Get(),0,16);
    recorder.list->CopyBufferRegion(readback.Get(),32,old_draw.index_resource.Get(),128,16);
    Require(SUCCEEDED(recorder.list->Close()),"copy list close");const auto token=gpu.Execute(recorder.list.Get());gpu.Complete(token);
    void* bytes{};D3D12_RANGE range{0,48};Require(SUCCEEDED(readback->Map(0,&range,&bytes)),"readback map");
    for(size_t i=0;i<48;++i)Require(static_cast<uint8_t*>(bytes)[i]==(i<16?37:i<32?93:11),"retained current resource bytes changed");
    D3D12_RANGE empty{};readback->Unmap(0,&empty);
    if(gpu.diagnostics)for(UINT64 i=0;i<gpu.diagnostics->GetNumStoredMessages();++i){SIZE_T size{};
        gpu.diagnostics->GetMessage(i,nullptr,&size);std::vector<uint8_t> data(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(data.data());
        Require(SUCCEEDED(gpu.diagnostics->GetMessage(i,message,&size)),"diagnostic read");
        Require(message->Severity!=D3D12_MESSAGE_SEVERITY_ERROR && message->Severity!=D3D12_MESSAGE_SEVERITY_CORRUPTION,"native GPU error");}
    gpu.Close();
}
}
int main(int argc,char** argv){try{Inputs();Geometry(argc<2 || !std::strcmp(argv[1],"warp"));
    std::puts("PASS current model inputs and native geometry: material membership, patch identity, policy, versions, withdrawal, same allocations and completed GPU copies");return 0;
}catch(const std::exception& error){std::fprintf(stderr,"FAIL %s\n",error.what());return 1;}}
