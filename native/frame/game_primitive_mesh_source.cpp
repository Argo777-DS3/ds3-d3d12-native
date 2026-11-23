#include "game_primitive_mesh_source.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <source_location>
#include <string>

namespace ds3native::bootstrap {
namespace {
void Require(bool value,const std::source_location location=std::source_location::current()){
    if(!value)throw std::runtime_error("unsupported CPU primitive staging transaction line"+std::to_string(location.line()));
}
uint32_t FieldBytes(uint32_t attributes) {
    constexpr uint32_t widths[]{1,1,2,2,4,4};
    const auto type=(attributes>>4)&15,components=attributes&15;
    Require(type>=1 && type<=6 && components>=1 && components<=4 && !(attributes&0xFFFF0000));
    return widths[type-1]*components;
}
void Validate(const CpuPrimitiveVertexFormat& format) {
    Require(format.stride && format.stride<=4096 && !format.elements.empty() && format.elements.size()<=16);
    for(size_t i=0;i<format.elements.size();++i) {
        const auto& a=format.elements[i];const auto bytes=FieldBytes(a.attributes);
        Require((a.attributes&0xFF00) && a.offset<=format.stride && bytes<=format.stride-a.offset);
        for(size_t j=0;j<i;++j) {
            const auto& b=format.elements[j];
            Require((a.attributes&0xFF00)!=(b.attributes&0xFF00));
            Require(a.offset>=b.offset+FieldBytes(b.attributes) || b.offset>=a.offset+bytes);
        }
    }
}
enum class Conversion {Copy,Position,Color};
struct Field {uint32_t source,destination,bytes;Conversion conversion;};
uint16_t Index(std::span<const uint8_t> bytes,size_t at){return uint16_t(uint16_t(bytes[at])|(uint16_t(bytes[at+1])<<8));}
}
std::shared_ptr<const PrimitiveGeometry> ProduceStagedPrimitiveGeometry(
    PrimitiveGeometryStorage& storage,uint64_t key,const CpuPrimitiveVertexFormat& source,
    const CpuPrimitiveVertexFormat& destination,std::span<const CpuStagedPrimitiveMesh> meshes) {
    Require(key && !meshes.empty() && meshes.size()<=256);Validate(source);Validate(destination);
    std::vector<Field> fields;
    for(const auto& element:destination.elements) {
        const auto found=std::find_if(source.elements.begin(),source.elements.end(),[&](const auto& value){return (value.attributes&0xFF00)==(element.attributes&0xFF00);});
        Require(found!=source.elements.end());Conversion conversion{};
        if(found->attributes==element.attributes)conversion=Conversion::Copy;
        else if(found->attributes==0x132 && element.attributes==0x162)conversion=Conversion::Position;
        else if((found->attributes==0x251 && element.attributes==0x214) || (found->attributes==0x214 && element.attributes==0x251))conversion=Conversion::Color;
        else Require(false);
        fields.push_back({found->offset,element.offset,FieldBytes(found->attributes),conversion});
    }
    uint64_t vertices{},indices{};
    for(const auto& mesh:meshes) {
        Require(mesh.vertices && mesh.vertex_bytes.size()==uint64_t(mesh.vertices)*source.stride && mesh.index_bytes.size()==uint64_t(mesh.indices)*2);
        vertices+=mesh.vertices;indices+=mesh.indices;
        Require(vertices<=65536 && indices*2<=16u*1024u*1024u && vertices*destination.stride<=16u*1024u*1024u);
        for(size_t i=0;i<mesh.index_bytes.size();i+=2)Require(Index(mesh.index_bytes,i)<mesh.vertices);
    }
    Require(indices);
    // Physical padding is zero; every declared field comes from real input.
    std::vector<uint8_t> output_vertices(static_cast<size_t>(vertices*destination.stride)),output_indices(static_cast<size_t>(indices*2));
    size_t vertex_base{},index_base{};
    for(const auto& mesh:meshes) {
        for(size_t vertex=0;vertex<mesh.vertices;++vertex)for(const auto& field:fields) {
            const auto* input=mesh.vertex_bytes.data()+vertex*source.stride+field.source;
            auto* output=output_vertices.data()+(vertex_base+vertex)*destination.stride+field.destination;
            if(field.conversion==Conversion::Copy)std::memcpy(output,input,field.bytes);
            else if(field.conversion==Conversion::Position) {
                int16_t position[2]{};std::memcpy(position,input,sizeof(position));
                const float converted[]{static_cast<float>(position[0]),static_cast<float>(position[1])};std::memcpy(output,converted,sizeof(converted));
            } else {output[0]=input[2];output[1]=input[1];output[2]=input[0];output[3]=input[3];}
        }
        for(size_t index=0;index<mesh.indices;++index) {
            const auto rebased=static_cast<uint32_t>(vertex_base)+Index(mesh.index_bytes,index*2);
            Require(rebased<=65535);output_indices[(index_base+index)*2]=static_cast<uint8_t>(rebased);
            output_indices[(index_base+index)*2+1]=static_cast<uint8_t>(rebased>>8);
        }
        vertex_base+=mesh.vertices;index_base+=mesh.indices;
    }
    auto writer=storage.Allocate(key,destination.stride,static_cast<uint32_t>(vertices),static_cast<uint32_t>(indices));
    std::memcpy(writer->Vertices(),output_vertices.data(),output_vertices.size());std::memcpy(writer->Indices(),output_indices.data(),output_indices.size());
    return storage.Seal(writer);
}
}
