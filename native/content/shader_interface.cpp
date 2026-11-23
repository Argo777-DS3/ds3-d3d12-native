#include "shader_interface.h"
#include <algorithm>
#include <cstring>
#include <mutex>
#include <set>
#include <span>
#include <stdexcept>
#include <unordered_map>

namespace ds3content {
namespace {
using Bytes=std::span<const uint8_t>;
void Require(bool ok,const char* what){if(!ok)throw std::runtime_error(what);}
bool Fits(size_t at,size_t bytes,size_t total){return at<=total && bytes<=total-at;}
uint32_t Word(Bytes bytes,size_t at) {
    Require(Fits(at,4,bytes.size()),"shader interface word extent");
    uint32_t value{};for(size_t i=0;i<4;++i)value|=uint32_t(bytes[at+i])<<(8*i);return value;
}
bool Magic(Bytes bytes,size_t at,const char* text){return Fits(at,4,bytes.size()) && std::equal(bytes.begin()+at,bytes.begin()+at+4,text);}
std::string Name(Bytes bytes,size_t at) {
    Require(at<bytes.size(),"shader interface name pointer");
    std::string name;
    for(size_t i=at;i<bytes.size() && i-at<=2048;++i) {
        const auto c=bytes[i];if(!c){Require(!name.empty(),"shader interface empty name");return name;}
        Require(c>=32 && c<127,"shader interface name encoding");name.push_back(static_cast<char>(c));
    }
    throw std::runtime_error("shader interface unterminated name");
}
std::vector<ShaderSignatureDeclaration> Signature(Bytes bytes) {
    Require(bytes.size()>=8 && Word(bytes,4)==8,"unsupported shader signature envelope");
    const auto count=Word(bytes,0);
    Require(count<=128 && Fits(8,size_t(count)*24,bytes.size()),"shader signature table extent");
    std::vector<ShaderSignatureDeclaration> result;std::set<std::pair<std::string,uint32_t>> names;
    for(uint32_t i=0;i<count;++i) {
        const size_t at=8+size_t(i)*24;const auto name_at=Word(bytes,at),packed=Word(bytes,at+20);
        Require(name_at>=8+size_t(count)*24 && !(packed&0xFFFF0000),"shader signature name/mask extent");
        ShaderSignatureDeclaration row{Name(bytes,name_at),Word(bytes,at+4),Word(bytes,at+8),Word(bytes,at+12),
            Word(bytes,at+16),static_cast<uint8_t>(packed&255),static_cast<uint8_t>((packed>>8)&255)};
        Require(row.mask && row.mask<=15 && row.usage_mask<=15 && row.component_type>=1 && row.component_type<=3 &&
            names.emplace(row.semantic,row.semantic_index).second,"shader signature duplicate/type/mask");
        result.push_back(std::move(row));
    }
    return result;
}
void Reflection(Bytes bytes,ShaderStage stage,ShaderInterface& result) {
    Require(bytes.size()>=60 && Word(bytes,16)==((stage==ShaderStage::Vertex?0xFFFEu:stage==ShaderStage::Compute?0x4353u:stage==ShaderStage::Hull?0x4853u:stage==ShaderStage::Domain?0x4453u:0xFFFFu)<<16|0x0500u) &&
        Magic(bytes,28,"RD11") && Word(bytes,32)==60 && Word(bytes,36)==24 && Word(bytes,40)==32 &&
        Word(bytes,44)==40 && Word(bytes,48)==36 && Word(bytes,52)==12 && !Word(bytes,56),
        "unsupported SM5 reflection envelope");
    const auto cb_count=Word(bytes,0),cb_at=Word(bytes,4),resource_count=Word(bytes,8),resource_at=Word(bytes,12);
    Require(cb_count<=128 && resource_count<=256 && Fits(cb_at,size_t(cb_count)*24,bytes.size()) &&
        Fits(resource_at,size_t(resource_count)*32,bytes.size()),"shader reflection table extent");
    std::set<std::string> resource_names,buffer_names;
    for(uint32_t i=0;i<resource_count;++i) {
        const size_t at=resource_at+size_t(i)*32;
        ShaderResourceDeclaration row{Name(bytes,Word(bytes,at)),Word(bytes,at+4),Word(bytes,at+8),Word(bytes,at+12),
            Word(bytes,at+16),Word(bytes,at+20),Word(bytes,at+24),Word(bytes,at+28)};
        Require(row.count && row.count<=65536 && row.shader_register<=65536-row.count &&
            resource_names.insert(row.name).second,"shader resource range/duplicate");
        result.resources.push_back(std::move(row));
    }
    for(uint32_t i=0;i<cb_count;++i) {
        const size_t at=cb_at+size_t(i)*24;const auto count=Word(bytes,at+4),variables_at=Word(bytes,at+8);
        ShaderConstantDeclaration row{Name(bytes,Word(bytes,at)),Word(bytes,at+12),Word(bytes,at+16),Word(bytes,at+20),{}};
        Require(row.declared_bytes<=65536 && count<=4096 && Fits(variables_at,size_t(count)*40,bytes.size()) &&
            buffer_names.insert(row.name).second,"shader constant table bound/duplicate");
        std::set<std::string> variable_names;std::vector<std::pair<uint32_t,uint32_t>> ranges;
        for(uint32_t j=0;j<count;++j) {
            const size_t va=variables_at+size_t(j)*40;
            ShaderVariableDeclaration variable{Name(bytes,Word(bytes,va)),Word(bytes,va+4),Word(bytes,va+8),Word(bytes,va+12)};
            Require(variable.bytes && variable.byte_offset<=row.declared_bytes && variable.bytes<=row.declared_bytes-variable.byte_offset &&
                Fits(Word(bytes,va+16),36,bytes.size()) && (!Word(bytes,va+20) || Fits(Word(bytes,va+20),variable.bytes,bytes.size())) &&
                variable_names.insert(variable.name).second,"shader variable extent/type/default/duplicate");
            const auto type=Word(bytes,va+16),kind=Word(bytes,type),dimensions=Word(bytes,type+4),array=Word(bytes,type+8);
            variable.value_class=static_cast<uint16_t>(kind&65535);variable.scalar_type=static_cast<uint16_t>(kind>>16);
            variable.rows=static_cast<uint16_t>(dimensions&65535);variable.columns=static_cast<uint16_t>(dimensions>>16);
            variable.elements=static_cast<uint16_t>(array&65535);variable.members=static_cast<uint16_t>(array>>16);
            ranges.emplace_back(variable.byte_offset,variable.byte_offset+variable.bytes);row.variables.push_back(std::move(variable));
        }
        std::sort(ranges.begin(),ranges.end());
        for(size_t j=1;j<ranges.size();++j)Require(ranges[j-1].second<=ranges[j].first,"shader overlapping variables");
        result.constants.push_back(std::move(row));
    }
}
}
ShaderInterface InspectShaderInterface(const CpuShaderProgram& program) {
    const Bytes bytes(program.bytecode);
    Require((program.stage==ShaderStage::Vertex || program.stage==ShaderStage::Pixel || program.stage==ShaderStage::Compute || program.stage==ShaderStage::Hull || program.stage==ShaderStage::Domain) &&   // I-0064: tessellation stages (RDEF targets HS 0x4853, DS 0x4453)
        program.model_major==5 && !program.model_minor &&
        bytes.size()>=32 && bytes.size()<=1024*1024 && Magic(bytes,0,"DXBC") && Word(bytes,20)==1 && Word(bytes,24)==bytes.size(),
        "shader interface stage/model/container");
    const auto count=Word(bytes,28);Require(count && count<=64 && Fits(32,size_t(count)*4,bytes.size()),"shader chunk directory");
    ShaderInterface result{};result.stage=program.stage;Bytes reflection,input,output;unsigned programs{};
    std::vector<std::pair<size_t,size_t>> ranges;
    for(uint32_t i=0;i<count;++i) {
        const auto at=Word(bytes,32+size_t(i)*4);
        Require(at>=32+size_t(count)*4 && Fits(at,8,bytes.size()),"shader chunk pointer");
        const auto size=Word(bytes,size_t(at)+4);Require(Fits(size_t(at)+8,size,bytes.size()),"shader chunk extent");
        ranges.emplace_back(at,size_t(at)+8+size);const auto body=bytes.subspan(size_t(at)+8,size);
        if(Magic(bytes,at,"RDEF")){Require(reflection.empty(),"duplicate shader reflection");reflection=body;}
        else if(Magic(bytes,at,"ISGN")){Require(input.empty(),"duplicate shader input signature");input=body;}
        else if(Magic(bytes,at,"OSGN")){Require(output.empty(),"duplicate shader output signature");output=body;}
        else if(Magic(bytes,at,"SHDR") || Magic(bytes,at,"SHEX")) {
            Require(size>=8 && !(size&3) && Word(body,4)==size/4 && Word(body,0)==(uint32_t(program.stage)<<16|0x50u),
                "shader program stage/model/extent");++programs;
        }
    }
    std::sort(ranges.begin(),ranges.end());
    for(size_t i=1;i<ranges.size();++i)Require(ranges[i-1].second<=ranges[i].first,"shader overlapping chunks");
    Require(programs==1 && !reflection.empty() && !input.empty() && !output.empty(),"shader interface required chunks");
    Reflection(reflection,program.stage,result);result.inputs=Signature(input);result.outputs=Signature(output);return result;
}
std::shared_ptr<const ShaderInterface> InspectShaderInterfaceShared(const CpuShaderProgram& program) {
    // Bucket key: stage, model, size and the container checksum (bytes 4..20).
    // Equality of the whole bytecode decides a hit; the key only selects a bucket.
    uint64_t key=uint64_t(program.stage)<<56^uint64_t(program.model_major)<<48^uint64_t(program.model_minor)<<40^program.bytecode.size();
    if(program.bytecode.size()>=20){uint64_t a{},b{};std::memcpy(&a,program.bytecode.data()+4,8);std::memcpy(&b,program.bytecode.data()+12,8);
        key^=a*0x9E3779B97F4A7C15ull^b;}
    struct Entry {ShaderStage stage;uint8_t major,minor;std::vector<uint8_t> bytecode;std::shared_ptr<const ShaderInterface> value;};
    struct Cache {std::mutex lock;std::unordered_multimap<uint64_t,Entry> entries;size_t bytes{};};
    static auto& cache=*new Cache;   // never destroyed: a worker still decoding at module detach finds it intact
    const auto find=[&]()->std::shared_ptr<const ShaderInterface>{
        for(auto [at,end]=cache.entries.equal_range(key);at!=end;++at){const auto& e=at->second;
            if(e.stage==program.stage && e.major==program.model_major && e.minor==program.model_minor && e.bytecode==program.bytecode)return e.value;}
        return {};};
    {const std::lock_guard guard(cache.lock);if(auto hit=find())return hit;}
    auto value=std::make_shared<const ShaderInterface>(InspectShaderInterface(program));
    Entry entry{program.stage,program.model_major,program.model_minor,program.bytecode,value};
    const std::lock_guard guard(cache.lock);
    if(auto hit=find())return hit;   // another thread decoded the same program meanwhile
    // Bounded by entries and retained bytecode; results already handed out stay owned by their holders.
    if(cache.entries.size()>=4096 || cache.bytes+entry.bytecode.size()>(16u<<20)){cache.entries.clear();cache.bytes=0;}
    cache.bytes+=entry.bytecode.size();cache.entries.emplace(key,std::move(entry));return value;
}
}
