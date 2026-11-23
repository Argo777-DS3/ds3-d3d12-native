#include "game_ui_mesh_output.h"
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>
using namespace ds3native::bootstrap;
namespace {
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
struct Format {uint32_t stride,pad;const CpuPrimitiveVertexElement* elements;};
struct Fill {uint32_t vertices,indices;const Format* format;uint32_t fills[2],flags,ordinal;};
static_assert(sizeof(Fill)==32 && sizeof(Format)==16);
struct Fixture {
    // Synthetic CPU ABI/control data only, never game assets or native inputs.
    std::array<CpuPrimitiveVertexElement,5> elements{{{0,0x132},{4,0x251},{8,0x1211},{9,0x2211},{}}};
    Format format{12,0,elements.data()};
    Fill fill{3,3,&format,{7,9},1,0};
    std::array<float,8> matrix{1,0,0,17,0,1,0,-5};
    std::array<uint8_t,36> vertices{};
    std::array<uint16_t,3> indices{0,2,1};
    Fixture(){for(size_t n=0;n<vertices.size();++n)vertices[n]=static_cast<uint8_t>(n+1);}
};
struct Abi {
    uintptr_t object;const uintptr_t* table;
    explicit Abi(NativeCpuMeshOutput& output):object(output.Interface()),table(*reinterpret_cast<const uintptr_t**>(object)){}
    bool Begin(Fixture& f,uint64_t count=1){return reinterpret_cast<uint8_t(*)(uintptr_t,uintptr_t,uint64_t,uintptr_t)>(table[1])(object,reinterpret_cast<uintptr_t>(&f.fill),count,reinterpret_cast<uintptr_t>(f.matrix.data()))!=0;}
    void Vertices(Fixture& f,uint32_t offset=0,uint32_t count=3,uint32_t fill=0){reinterpret_cast<void(*)(uintptr_t,uint32_t,uint32_t,uintptr_t,uint32_t)>(table[3])(object,fill,offset,reinterpret_cast<uintptr_t>(f.vertices.data()+size_t(offset)*12),count);}
    void Indices(Fixture& f){reinterpret_cast<void(*)(uintptr_t,uint32_t,uint32_t,uintptr_t,uint32_t)>(table[4])(object,0,0,reinterpret_cast<uintptr_t>(f.indices.data()),3);}
    void End(){reinterpret_cast<void(*)(uintptr_t)>(table[2])(object);}
    void Complete(Fixture& f){Require(Begin(f),"control Begin");Vertices(f,2,1);Indices(f);Vertices(f,0,2);End();}
};
void Expect(NativeCpuMeshOutput& native,CpuUiMeshOutputStatus status){
    CpuUiMeshOutput output;output.vertices=733;output.vertex_bytes={37};
    Require(native.Resolve(output)==status,"unexpected output status");
    Require(output.vertices==733 && output.vertex_bytes==std::vector<uint8_t>{37},"refusal changed destination");
}
void Generator(uintptr_t fixture,uintptr_t renderer,uintptr_t mesh,uintptr_t output,uint32_t flags){
    Require(renderer==31 && mesh==47 && flags==5,"generator argument ABI");
    auto& f=*reinterpret_cast<Fixture*>(fixture);auto table=*reinterpret_cast<const uintptr_t**>(output);
    Require(reinterpret_cast<uint8_t(*)(uintptr_t,uintptr_t,uint64_t,uintptr_t)>(table[1])(output,reinterpret_cast<uintptr_t>(&f.fill),1,reinterpret_cast<uintptr_t>(f.matrix.data()))!=0,"generator Begin");
    reinterpret_cast<void(*)(uintptr_t,uint32_t,uint32_t,uintptr_t,uint32_t)>(table[3])(output,0,0,reinterpret_cast<uintptr_t>(f.vertices.data()),3);
    reinterpret_cast<void(*)(uintptr_t,uint32_t,uint32_t,uintptr_t,uint32_t)>(table[4])(output,0,0,reinterpret_cast<uintptr_t>(f.indices.data()),3);
    reinterpret_cast<void(*)(uintptr_t)>(table[2])(output);
}
}
int main(){try{
    Fixture f;const auto expected=f.vertices;NativeCpuMeshOutput native;Abi abi(native);abi.Complete(f);
    // Metadata and chunks must survive source reuse after the synchronous call.
    f.vertices.fill(255);f.indices.fill(65535);f.matrix.fill(0);f.elements[0]={9,77};f.fill.vertices=88;
    CpuUiMeshOutput owned;Require(native.Resolve(owned)==CpuUiMeshOutputStatus::Ready,"complete output missing");
    Require(owned.vertices==3 && owned.indices==3 && owned.format.stride==12 && owned.format.elements[0].attributes==0x132 && owned.fills[0]==7 && owned.fills[1]==9 && owned.flags==1 && owned.mesh_ordinal==0 && owned.transform[3]==17 && owned.transform[7]==-5,"borrowed metadata escaped");
    Require(owned.vertex_bytes==std::vector<uint8_t>(expected.begin(),expected.end()) && owned.index_bytes==std::vector<uint8_t>({0,0,2,0,1,0}),"chunk/count ABI or data lifetime");
    {Fixture x;NativeCpuMeshOutput n;Expect(n,CpuUiMeshOutputStatus::Missing);Abi a(n);Require(a.Begin(x),"partial Begin");a.Vertices(x,0,2);a.Indices(x);a.End();Expect(n,CpuUiMeshOutputStatus::Missing);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);Require(a.Begin(x),"unfinished Begin");a.Vertices(x);a.Indices(x);Expect(n,CpuUiMeshOutputStatus::Missing);}
    {Fixture x;x.indices[2]=3;NativeCpuMeshOutput n;Abi a(n);a.Complete(x);Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);Require(!a.Begin(x,2),"multi-fill accepted");Require(!a.Begin(x),"unsupported output restarted");a.End();Expect(n,CpuUiMeshOutputStatus::Unsupported);}
    {Fixture x;x.matrix[1]=std::numeric_limits<float>::infinity();NativeCpuMeshOutput n;Abi a(n);Require(!a.Begin(x),"nonfinite transform accepted");Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;x.format.elements=reinterpret_cast<const CpuPrimitiveVertexElement*>(1);NativeCpuMeshOutput n;Abi a(n);Require(!a.Begin(x),"unreadable format accepted");Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);Require(a.Begin(x),"bad fill Begin");a.Vertices(x,0,3,1);Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);Require(a.Begin(x),"bounds Begin");a.Vertices(x,2,2);Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);a.End();Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);Require(a.Begin(x),"duplicate Begin");Require(!a.Begin(x),"duplicate Begin accepted");Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);a.Complete(x);a.End();Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);a.Complete(x);a.Vertices(x);Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);Require(a.Begin(x),"thread Begin");std::thread foreign([&]{a.Vertices(x);});foreign.join();Expect(n,CpuUiMeshOutputStatus::Invalid);}
    {Fixture x;NativeCpuMeshOutput n;Abi a(n);a.Complete(x);CpuUiMeshOutput ignored;std::thread foreign([&]{Require(n.Resolve(ignored)==CpuUiMeshOutputStatus::Invalid,"foreign Resolve accepted");});foreign.join();Require(n.Resolve(ignored)==CpuUiMeshOutputStatus::Ready,"foreign reader corrupted publication");}
    {Fixture x;CpuUiMeshOutput result;Require(ProduceNativeCpuUiMesh(&Generator,reinterpret_cast<uintptr_t>(&x),31,47,5,result)==CpuUiMeshOutputStatus::Ready && result.vertex_bytes.size()==36,"generator wrapper ABI");result.vertices=599;Require(ProduceNativeCpuUiMesh(nullptr,1,2,3,0,result)==CpuUiMeshOutputStatus::Invalid && result.vertices==599,"invalid generator changed output");}
    std::puts("PASS owned CPU mesh output: five-method generator ABI, reordered chunks, deep format/transform lifetime, full coverage, atomic missing/unsupported/index/range/thread refusal, duplicate and post-End callbacks rejected");return 0;
}catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}}
