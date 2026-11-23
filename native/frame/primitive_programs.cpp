#include "primitive_programs.h"
#include <windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace ds3native::frame {
namespace {
struct Recipe {uint32_t selector,record;int32_t vertex,pixel;bool available;};
struct ProgramPin {uint32_t stage;int32_t index;uint32_t bytes;const char* sha256;};
#include "primitive_program_catalog.inc"
uint64_t Identity(uint32_t stage,int32_t index){return (uint64_t(stage==0)<<32)|uint64_t(uint32_t(index)+1);}
void Require(bool ok){if(!ok)throw std::runtime_error("invalid owned primitive programme bank");}
struct Reader {
    std::span<const uint8_t> data;size_t position{};
    std::span<const uint8_t> Take(size_t bytes){Require(position<=data.size() && bytes<=data.size()-position);auto value=data.subspan(position,bytes);position+=bytes;return value;}
    uint32_t Word(){auto bytes=Take(4);uint32_t v{};for(size_t i=0;i<4;++i)v|=uint32_t(bytes[i])<<(8*i);return v;}
    uint64_t Wide(){auto lo=Word();return uint64_t(Word())<<32|lo;}
};
bool ExactHash(std::span<const uint8_t> bytes,const char* expected){
    std::array<uint8_t,32> digest{};
    if(BCryptHash(BCRYPT_SHA256_ALG_HANDLE,nullptr,0,const_cast<PUCHAR>(bytes.data()),static_cast<ULONG>(bytes.size()),digest.data(),static_cast<ULONG>(digest.size())))return false;
    constexpr char hex[]="0123456789ABCDEF";
    for(size_t i=0;i<digest.size();++i)if(expected[2*i]!=hex[digest[i]>>4] || expected[2*i+1]!=hex[digest[i]&15])return false;
    return expected[64]==0;
}
bool ProgramStage(std::span<const uint8_t> bytes,uint32_t stage){
    // Whole-byte source pins qualify the exact container; independently check
    // its declared extent and code token before publishing a CPU programme.
    if(bytes.size()<32 || std::memcmp(bytes.data(),"DXBC",4))return false;
    Reader r{bytes};r.position=20;if(r.Word()!=1 || r.Word()!=bytes.size())return false;
    const auto count=r.Word();if(!count || count>64)return false;
    uint32_t programs{};
    for(uint32_t i=0;i<count;++i){
        r.position=32+size_t(i)*4;const auto at=r.Word();r.position=at;const auto tag=r.Take(4);const auto size=r.Word();const auto body=r.Take(size);
        if(!std::memcmp(tag.data(),"SHDR",4) || !std::memcmp(tag.data(),"SHEX",4)){
            Reader p{body};const auto token=p.Word();if((token>>16)!=stage || (token&0xFFFF)!=0x50)return false;++programs;
        }
    }
    return programs==1;
}
const Recipe* FindRecipe(uint32_t selector){
    const auto found=std::lower_bound(std::begin(recipes),std::end(recipes),selector,[](const auto& r,uint32_t s){return r.selector<s;});
    return found!=std::end(recipes) && found->selector==selector?found:nullptr;
}
}
PrimitiveProgramStatus DecodePrimitiveProgramBank(std::span<const uint8_t> bytes,PrimitiveProgramVersion current,PrimitiveProgramBank& output){
    if(!current.incarnation || !current.generation || bytes.size()>32u*1024u*1024u)return PrimitiveProgramStatus::Invalid;
    try{
        Reader reader{bytes};auto magic=reader.Take(8);Require(!std::memcmp(magic.data(),"PPBK0001",8));
        Require(reader.Word()==1 && reader.Word()==3);
        PrimitiveProgramBank candidate;candidate.version_={reader.Wide(),reader.Wide()};
        if(candidate.version_!=current)return PrimitiveProgramStatus::Stale;
        const auto count=reader.Word();Require(!reader.Word() && count<=std::size(program_pins));
        for(uint32_t i=0;i<count;++i){
            const auto index=reader.Word(),stage=reader.Word(),size=reader.Word();Require(index<=INT32_MAX && stage<=1);
            const auto pin=std::find_if(std::begin(program_pins),std::end(program_pins),[&](const auto& p){return p.stage==stage && p.index==static_cast<int32_t>(index);});
            Require(pin!=std::end(program_pins) && size==pin->bytes);
            const auto payload=reader.Take(size);Require(ExactHash(payload,pin->sha256) && ProgramStage(payload,stage));
            const auto identity=Identity(stage,static_cast<int32_t>(index));
            ds3content::CpuShaderProgram program{static_cast<int32_t>(index),"primitive-"+std::string(stage?"vs-":"ps-")+std::to_string(index),
                stage?ds3content::ShaderStage::Vertex:ds3content::ShaderStage::Pixel,5,0,{payload.begin(),payload.end()}};
            Require(candidate.programs_.emplace(identity,PrimitiveProgram{identity,std::move(program)}).second);
        }
        Require(reader.position==bytes.size());output=std::move(candidate);return PrimitiveProgramStatus::Ready;
    }catch(const std::runtime_error&){return PrimitiveProgramStatus::Invalid;}
}
PrimitiveProgramStatus DecodeOriginalPrimitiveProgramBank(std::span<const uint8_t> pe,PrimitiveProgramVersion current,PrimitiveProgramBank& output){
    if(!current.incarnation || !current.generation || pe.size()!=88960032 ||
        !ExactHash(pe,"EF5E07C55222F14FFDDECF2C724A0C2A95CEF0D4DA0E075B0DE0BB108B69498C"))return PrimitiveProgramStatus::Invalid;
    try {
        Reader image{pe};image.position=60;const auto header=image.Word();image.position=header;
        Require(image.Word()==0x4550);const auto machine_and_sections=image.Word();
        Require((machine_and_sections&0xFFFF)==0x8664);const auto sections=machine_and_sections>>16;
        image.position=header+20;const auto optional_and_flags=image.Word();const auto optional=optional_and_flags&0xFFFF;
        Require(sections && sections<=96 && optional>=112);
        image.position=header+24;Require((image.Word()&0xFFFF)==0x20B);
        image.position=header+48;const auto base=image.Wide();Require(base==0x140000000);
        struct Section {uint32_t address,size,offset;};std::vector<Section> spans;
        for(uint32_t section=0;section<sections;++section) {
            image.position=header+24+optional+section*40+12;
            spans.push_back({image.Word(),image.Word(),image.Word()});
        }
        const auto original=[&](uint64_t address,size_t bytes) {
            Require(address>=base && address-base<=UINT32_MAX);const auto rva=address-base;
            size_t offset{},matches{};
            for(const auto& section:spans)if(rva>=section.address && rva-section.address<=section.size && bytes<=section.size-(rva-section.address)) {
                offset=static_cast<size_t>(section.offset+rva-section.address);++matches;
            }
            Require(matches==1 && offset<=pe.size() && bytes<=pe.size()-offset);return pe.subspan(offset,bytes);
        };
        std::vector<uint8_t> bank{'P','P','B','K','0','0','0','1'};
        const auto word=[&](uint32_t value){for(unsigned byte=0;byte<4;++byte)bank.push_back(static_cast<uint8_t>(value>>(byte*8)));};
        const auto wide=[&](uint64_t value){word(static_cast<uint32_t>(value));word(static_cast<uint32_t>(value>>32));};
        word(1);word(3);wide(current.incarnation);wide(current.generation);word(static_cast<uint32_t>(std::size(program_pins)));word(0);
        std::map<uint64_t,PrimitiveUniformLayout> layouts;
        std::map<uint64_t,PrimitiveVertexLayout> vertex_layouts;
        for(const auto& pin:program_pins) {
            // Qualified1F81A90 static CPU tables: stage1 vertex, stage0 pixel.
            Reader table{original(base+(pin.stage?0x46F90C0:0x46FC330)+uint64_t(pin.index)*8,8)};
            const auto descriptor_address=table.Wide();
            Reader descriptor{original(descriptor_address+0x10,24)};
            const auto uniform_data=original(descriptor.Wide(),250),semantic_data=original(descriptor.Wide(),75);
            PrimitiveUniformLayout layout;layout.installed=true;
            std::memcpy(layout.uniforms.data(),uniform_data.data(),uniform_data.size());
            std::memcpy(layout.semantics.data(),semantic_data.data(),semantic_data.size());
            Require(layouts.emplace(Identity(pin.stage,pin.index),layout).second);
            const auto payload=original(descriptor.Wide(),pin.bytes);
            word(static_cast<uint32_t>(pin.index));word(pin.stage);word(pin.bytes);bank.insert(bank.end(),payload.begin(),payload.end());
            if(pin.stage){
                //1F7FFD0 reads these CPU records before creating any original
                // input-layout object: semantic/name/index/format, slot0,
                // per-vertex data and appended offsets. Own only that metadata.
                PrimitiveVertexLayout vertex;vertex.installed=true;vertex.supported=true;
                const auto count=original(descriptor_address+0x2C,1)[0];Require(count<=8);
                uint32_t offset{};
                for(uint32_t element=0;element<count;++element){
                    Reader record{original(descriptor_address+0x40+uint64_t(element)*32,16)};
                    const auto name=original(record.Wide(),32);const auto end=std::find(name.begin(),name.end(),uint8_t{});
                    Require(end!=name.end() && end!=name.begin());
                    const auto index=record.Word();const auto format=static_cast<DXGI_FORMAT>(record.Word());
                    uint32_t bytes{},alignment{1};
                    if(format==DXGI_FORMAT_R32G32_FLOAT){bytes=8;alignment=4;}
                    else if(format==DXGI_FORMAT_R8G8B8A8_UNORM)bytes=4;
                    else if(format==DXGI_FORMAT_R8_UINT)bytes=1;
                    else vertex.supported=false;
                    offset=(offset+alignment-1)&~(alignment-1);
                    vertex.elements.push_back({std::string(name.begin(),end),index,offset,format});offset+=bytes;
                }
                vertex.stride=vertex.supported?offset:0;
                Require(vertex_layouts.emplace(Identity(pin.stage,pin.index),std::move(vertex)).second);
            }
        }
        PrimitiveProgramBank candidate;const auto status=DecodePrimitiveProgramBank(bank,current,candidate);
        if(status!=PrimitiveProgramStatus::Ready)return status;
        candidate.uniforms_=std::move(layouts);candidate.vertex_layouts_=std::move(vertex_layouts);output=std::move(candidate);return status;
    }catch(const std::runtime_error&){return PrimitiveProgramStatus::Invalid;}
}
PrimitiveProgramStatus SelectPrimitivePrograms(const PrimitiveProgramBank& bank,PrimitiveProgramVersion current,uint32_t source_mode,uint32_t selector,PrimitiveProgramPair& output){
    if(!current.incarnation || !current.generation || !bank.version_.incarnation)return PrimitiveProgramStatus::Invalid;
    if(bank.version_!=current)return PrimitiveProgramStatus::Stale;
    if(source_mode!=3)return PrimitiveProgramStatus::UnsupportedMode;
    const auto recipe=FindRecipe(selector);if(!recipe)return PrimitiveProgramStatus::UnknownSelector;
    if(!recipe->available)return PrimitiveProgramStatus::Missing;
    const auto a=bank.programs_.find(Identity(1,recipe->vertex)),b=bank.programs_.find(Identity(0,recipe->pixel));
    if(a==bank.programs_.end() || b==bank.programs_.end())return PrimitiveProgramStatus::Missing;
    PrimitiveProgramPair candidate{current,selector,recipe->record,source_mode,a->second,b->second,{},{},{}};
    const auto va=bank.uniforms_.find(a->first),pa=bank.uniforms_.find(b->first);
    if(va!=bank.uniforms_.end())candidate.vertex_uniforms=va->second;
    if(pa!=bank.uniforms_.end())candidate.pixel_uniforms=pa->second;
    const auto input=bank.vertex_layouts_.find(a->first);
    if(input!=bank.vertex_layouts_.end())candidate.vertex_layout=input->second;
    output=std::move(candidate);return PrimitiveProgramStatus::Ready;
}
}
