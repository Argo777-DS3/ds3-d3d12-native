#include "game_scene_geometry.h"
#include "../content/acquire.h"
#include <array>
#include <cstring>

namespace ds3native::ingress {
namespace {
template<class T>bool Load(uintptr_t address,T& value) noexcept {
    return ds3content::Read(address,&value,sizeof(value));
}
template<class T,size_t N>T Field(const std::array<uint8_t,N>& bytes,size_t offset) noexcept {
    T value{};std::memcpy(&value,bytes.data()+offset,sizeof(value));return value;
}
struct Tables {uintptr_t vptr{},storage{},meshes{},indices{},vertices{};uint32_t mesh_count{},vertex_count{};
    bool operator==(const Tables&)const=default;};
bool Snapshot(uintptr_t model,Tables& out) noexcept {
    return Load(model,out.vptr) && Load(model+0xE8,out.storage) &&
        Load(model+0xA0,out.mesh_count) && Load(model+0xA8,out.meshes) &&
        Load(model+0x120,out.indices) && Load(model+0x150,out.vertex_count) && Load(model+0x158,out.vertices);
}
bool CopyOnce(uintptr_t model,uintptr_t image,uintptr_t storage,uint32_t index_count,
    size_t source_bytes,std::vector<MeshSlice>& output,int& reason) {
    Tables before{},after{};
    if(!Snapshot(model,before) || before.vptr!=image+ds3content::model_vtable_rva ||
       before.storage!=storage || !before.mesh_count || before.mesh_count>kMaximumContentMeshes ||
       !before.vertex_count || before.vertex_count>4096 || !index_count || index_count>4096 ||
       !before.meshes || !before.indices || !before.vertices || !source_bytes || source_bytes>(32u<<20)){
        // 1199: the bound that refused (16 meshes, 17 vertex buffers, 18 index buffers, 1 anything else).
        reason=before.mesh_count>kMaximumContentMeshes?16:before.vertex_count>4096?17:index_count>4096?18:1;return false;}
    std::vector<MeshSlice> copied;copied.reserve(before.mesh_count);
    for(uint32_t mesh=0;mesh<before.mesh_count;++mesh){
        std::array<uint8_t,0x48> entry{};
        if(!ds3content::Read(before.meshes+uint64_t(mesh)*entry.size(),entry.data(),entry.size())){reason=2;return false;}
        const auto faces=Field<uint16_t>(entry,0x28),streams=Field<uint16_t>(entry,0x2A);
        const auto face_table=Field<uintptr_t>(entry,0x30),stream_table=Field<uintptr_t>(entry,0x38);
        // 1063: one or two vertex buffers (character parts: two, same vertex count).
        if(!faces || faces>64 || !streams || streams>2 || !face_table || !stream_table){reason=3;return false;}
        int32_t stream{};
        if(!Load(stream_table,stream) || stream<0 || uint32_t(stream)>=before.vertex_count){reason=4;return false;}
        std::array<uint8_t,0x90> vertex{};
        if(!ds3content::Read(before.vertices+uint64_t(stream)*vertex.size(),vertex.data(),vertex.size()) ||
           Field<uintptr_t>(vertex,0x70)!=storage || Field<int32_t>(vertex,0x6C)!=0){reason=5;return false;}
        const auto stride=Field<uint32_t>(vertex,0x68);
        // 1058: any 4-aligned stride; element offsets/widths are validated against
        // it at draw preparation (skinned character layouts are wider than 48).
        if(stride<12 || stride>256 || stride%4){reason=6;return false;}
        uint32_t offset2{},stride2{};
        if(streams==2){
            int32_t other{};std::array<uint8_t,0x90> second{};
            if(!Load(stream_table+4,other) || other<0 || uint32_t(other)>=before.vertex_count ||
               !ds3content::Read(before.vertices+uint64_t(other)*second.size(),second.data(),second.size()) ||
               Field<uintptr_t>(second,0x70)!=storage || Field<uint32_t>(second,0x7C)!=Field<uint32_t>(vertex,0x7C)){reason=15;return false;}
            stride2=Field<uint32_t>(second,0x68);offset2=Field<uint32_t>(second,0x78);
            if(stride2<4 || stride2>256 || stride2%4 || offset2%4 ||
               uint64_t(offset2)+uint64_t(Field<uint32_t>(second,0x7C))*stride2>source_bytes){reason=15;return false;}
        }
        bool selected{};MeshSlice slice{};
        for(uint32_t face=0;face<faces;++face){
            int32_t index{};if(!Load(face_table+uint64_t(face)*4,index) || index<0 || uint32_t(index)>=index_count){reason=7;return false;}
            std::array<uint8_t,0x20> indices{};
            if(!ds3content::Read(before.indices+uint64_t(index)*indices.size(),indices.data(),indices.size())){reason=8;return false;}
            if(Field<uint32_t>(indices,0)!=0)continue;
            // 1199: 19 a second LOD-0 face set; 9 with the topology (bits 8-15) and the index format (bits 16-23) refused.
            if(selected){reason=19;return false;}
            // 1202: 1FA030 writes format 0x39 (R16_UINT) for a 16-bit FLVER face set and 0x2A (R32_UINT) otherwise; the
            // view start (+8) is the face set's data offset divided by that width.
            const auto format=Field<uint32_t>(indices,8);
            if(Field<uint32_t>(indices,4)!=4 || (format!=0x39 && format!=0x2A)){
                reason=9|(Field<uint32_t>(indices,4)&0xFF)<<8|(format&0xFF)<<16;return false;}
            const uint32_t width=format==0x2A?4:2;
            const auto view=Field<uintptr_t>(indices,0x18);uintptr_t index_storage{};uint32_t start{};
            if(!view || !Load(view+0x10,index_storage) || index_storage!=storage || !Load(view+8,start)){reason=10;return false;}
            const uint64_t index_offset=(uint64_t(start)+Field<uint32_t>(indices,0x14))*width;
            if(index_offset>UINT32_MAX){reason=11;return false;}
            slice={Field<uint32_t>(vertex,0x78),Field<uint32_t>(vertex,0x7C),stride,
                static_cast<uint32_t>(index_offset),Field<uint32_t>(indices,0x10),Field<uint16_t>(entry,0)};
            slice.index_width=width;
            const uint64_t vertex_end=uint64_t(slice.vertex_offset)+uint64_t(slice.vertex_count)*stride;
            const uint64_t index_end=index_offset+uint64_t(slice.index_count)*width;
            if(!slice.vertex_count || !slice.index_count || slice.index_count%3 ||
               slice.vertex_offset%4 || vertex_end>source_bytes || index_end>source_bytes){reason=12;return false;}
            slice.vertex_offset2=offset2;slice.stride2=stride2;
            slice.cull_class=Field<uint32_t>(indices,0x0C);if(slice.cull_class>2)slice.cull_class=1;   // table 14473C9F8 has three columns
            selected=true;
        }
        if(!selected){reason=13;return false;}copied.push_back(slice);
    }
    if(!Snapshot(model,after) || before!=after){reason=14;return false;}
    output=std::move(copied);return true;
}
}
bool CopyModelGeometry(uintptr_t model,uintptr_t image,uintptr_t storage,
    uint32_t index_count,size_t source_bytes,std::vector<MeshSlice>& output,int* refusal) noexcept {
    try {
        if(!model || !image || !storage)return false;
        std::vector<MeshSlice> first,second;
        int reason{};
        if(!CopyOnce(model,image,storage,index_count,source_bytes,first,reason) ||
           !CopyOnce(model,image,storage,index_count,source_bytes,second,reason) || first.size()!=second.size()){if(refusal)*refusal=reason?reason:14;return false;}   // B9: a count changed between the copies is check 14 too
        for(size_t i=0;i<first.size();++i){const auto& a=first[i];const auto& b=second[i];
            // I-0120 B9: the second stream compared too, and a mismatch names its check (14, the copies' instability) as Snapshot's does.
            if(a.vertex_offset!=b.vertex_offset || a.vertex_count!=b.vertex_count || a.stride!=b.stride ||
               a.vertex_offset2!=b.vertex_offset2 || a.stride2!=b.stride2 ||
               a.index_offset!=b.index_offset || a.index_count!=b.index_count || a.material_id!=b.material_id || a.cull_class!=b.cull_class ||
               a.index_width!=b.index_width){if(refusal)*refusal=14;return false;}
        }
        output=std::move(first);return true;
    }catch(...){return false;}
}
}
