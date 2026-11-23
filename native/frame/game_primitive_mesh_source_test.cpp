#include "game_primitive_mesh_source.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
using namespace ds3native::bootstrap;
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F> void Refuses(F action){bool failed{};try{action();}catch(const std::runtime_error&){failed=true;}Require(failed,"invalid staging accepted");}
int main() {
    try {
        // Actual formats from reference11, synthetic boundary-value payloads.
        const CpuPrimitiveVertexFormat source{12,{{0,0x132},{4,0x251},{8,0x1211},{9,0x2211}}};
        const CpuPrimitiveVertexFormat destination{16,{{0,0x214},{4,0x2211},{7,0x1211},{8,0x162}}};
        std::array<uint8_t,24> vertices{0,128,255,127,0x19,0x53,0xB7,0xE1,11,29,99,88,255,255,0,0,0x31,0x62,0x93,0xC4,13,37,77,66};
        std::array<uint8_t,4> indices{1,0,0,0};
        const std::array<CpuStagedPrimitiveMesh,2> meshes{{{2,2,vertices,indices},{2,2,vertices,indices}}};
        PrimitiveGeometryStorage storage;auto lease=ProduceStagedPrimitiveGeometry(storage,41,source,destination,meshes);
        Require(lease->vertex_count==4 && lease->index_count==4 && lease->vertices.bytes.size()==64,"batch extent");
        const auto& bytes=lease->vertices.bytes;
        Require(bytes[0]==0xB7 && bytes[1]==0x53 && bytes[2]==0x19 && bytes[3]==0xE1 && bytes[4]==29 && bytes[7]==11,"color/factors");
        float positions[4]{};std::memcpy(positions,bytes.data()+8,8);std::memcpy(positions+2,bytes.data()+24,8);
        Require(positions[0]==-32768 && positions[1]==32767 && positions[2]==-1 && positions[3]==0,"signed positions");
        Require(lease->indices.bytes==std::vector<uint8_t>({1,0,0,0,3,0,2,0}),"local indices rebased");
        const CpuPrimitiveVertexFormat compact{12,{{0,0x2211},{3,0x1211},{4,0x162}}};
        auto compact_lease=ProduceStagedPrimitiveGeometry(storage,43,source,compact,meshes);
        Require(compact_lease->vertices.bytes[0]==29 && compact_lease->vertices.bytes[3]==11 && compact_lease->vertices.bytes.size()==48,"compact family");
        auto unknown=destination;unknown.elements[0].attributes|=0x10000;
        Refuses([&]{ProduceStagedPrimitiveGeometry(storage,41,source,unknown,meshes);});
        Require(storage.Acquire(41)==lease,"unsupported replaced current publication");
        indices[0]=2;Refuses([&]{ProduceStagedPrimitiveGeometry(storage,41,source,destination,meshes);});
        Require(storage.Acquire(41)==lease,"bad local index replaced publication");indices[0]=1;
        auto too_short=meshes;too_short[0].vertex_bytes=std::span(vertices).first(23);
        Refuses([&]{ProduceStagedPrimitiveGeometry(storage,41,source,destination,too_short);});
        auto overlap=destination;overlap.elements[3].offset=7;
        Refuses([&]{ProduceStagedPrimitiveGeometry(storage,41,source,overlap,meshes);});
        auto missing=source;missing.elements.pop_back();Refuses([&]{ProduceStagedPrimitiveGeometry(storage,41,missing,destination,meshes);});
        auto duplicate=source;duplicate.elements.push_back(source.elements[0]);Refuses([&]{ProduceStagedPrimitiveGeometry(storage,41,duplicate,destination,meshes);});
        vertices.fill(0);indices.fill(255);storage.Withdraw(41);
        Require(lease->vertices.bytes[0]==0xB7 && lease->indices.bytes[0]==1,"borrowed/owner lifetime");
        std::puts("PASS CPU staged primitive source: observed12/16-byte formats, signed boundaries, color/factors, multiple-mesh rebasing, atomic invalid/missing/layout refusal and owned lifetime");return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
