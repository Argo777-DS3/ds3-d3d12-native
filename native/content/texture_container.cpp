#include "texture_container.h"
#include "../third_party/zlib/zlib.h"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace ds3content {
namespace {
constexpr size_t Limit=128u*1024u*1024u;
void Require(bool value,const char* label){if(!value)throw std::runtime_error(label);}
bool Extent(size_t start,size_t length,size_t total){return start<=total && length<=total-start;}
using Bytes=std::span<const uint8_t>;
uint32_t Word(Bytes data,size_t offset,bool big=false) {
    Require(Extent(offset,4,data.size()),"CPU container field extent");uint32_t value{};
    for(size_t i=0;i<4;++i)value|=uint32_t(data[offset+i])<<((big?3-i:i)*8);
    return value;
}
bool Magic(Bytes data,size_t offset,const char* value) {
    return Extent(offset,4,data.size()) && std::equal(data.begin()+offset,data.begin()+offset+4,value);
}
std::vector<uint8_t> Expand(Bytes input) {
    Require(!input.empty() && input.size()<=Limit,"CPU container input bound");
    if(Magic(input,0,"TPF\0"))return {input.begin(),input.end()};
    Require(input.size()>=76 && Magic(input,0,"DCX\0") && Word(input,4,true)==0x10000 &&
        Word(input,8,true)==0x18 && Word(input,12,true)==0x24 && Word(input,16,true)==0x44 &&
        Word(input,20,true)==0x4C && Magic(input,24,"DCS\0") && Magic(input,36,"DCP\0") &&
        Magic(input,40,"DFLT") && Word(input,44,true)==0x20 && input[48]==9 &&
        !input[49] && !input[50] && !input[51] && !Word(input,52,true) && !Word(input,56,true) &&
        !Word(input,60,true) && Word(input,64,true)==0x10100 && Magic(input,68,"DCA\0") &&
        Word(input,72,true)==8,"unsupported CPU DCX envelope");
    const auto expanded=Word(input,28,true),compressed=Word(input,32,true);
    Require(expanded>0 && expanded<=Limit && compressed>0 && compressed==input.size()-76,"CPU DCX declared extent");
    std::vector<uint8_t> output(expanded);z_size_t output_size=expanded,input_size=compressed;
    const int result=uncompress2_z(output.data(),&output_size,input.data()+76,&input_size);
    Require(result==Z_OK && output_size==expanded && input_size==compressed,"CPU DCX incomplete corrupt or trailing stream");
    return output;
}
struct Name {std::string value;size_t begin{},end{};};
Name ReadName(Bytes data,size_t offset,uint8_t encoding) {
    Require(offset<data.size(),"CPU TPF name offset");std::wstring wide;size_t end=offset;
    if(encoding==1) {
        Require(!(offset&1),"CPU TPF UTF16 alignment");
        for(;;end+=2) {
            Require(Extent(end,2,data.size()) && wide.size()<=1024,"CPU TPF name termination");
            const auto unit=uint16_t(data[end])|(uint16_t(data[end+1])<<8);
            if(!unit){end+=2;break;}wide.push_back(static_cast<wchar_t>(unit));
        }
    } else {
        Require(encoding==0 || encoding==2,"CPU TPF name encoding");
        while(end<data.size() && data[end] && end-offset<=1024)++end;
        Require(end<data.size() && !data[end] && end>offset && end-offset<=1024,"CPU TPF ShiftJIS termination");
        const auto count=static_cast<int>(end-offset);
        const int size=MultiByteToWideChar(932,MB_ERR_INVALID_CHARS,reinterpret_cast<const char*>(data.data()+offset),count,nullptr,0);
        Require(size>0,"CPU TPF ShiftJIS name");wide.resize(static_cast<size_t>(size));
        Require(MultiByteToWideChar(932,MB_ERR_INVALID_CHARS,reinterpret_cast<const char*>(data.data()+offset),count,wide.data(),size)==size,"CPU TPF ShiftJIS conversion");++end;
    }
    Require(!wide.empty() && wide.size()<=1024,"CPU TPF empty or overlong name");
    const int size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide.data(),static_cast<int>(wide.size()),nullptr,0,nullptr,nullptr);
    Require(size>0 && size<=1024,"CPU TPF UTF8 name extent");std::string value(static_cast<size_t>(size),'\0');
    Require(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,wide.data(),static_cast<int>(wide.size()),value.data(),size,nullptr,nullptr)==size,"CPU TPF invalid Unicode name");
    return {std::move(value),offset,end};
}
struct Record {
    size_t start{},length{};Name name;uint8_t format{},type{},mips{};
    std::optional<CpuTextureFloatMetadata> float_metadata;
};
// Optimisation review: the whole container is validated as before, but only the requested entries are returned (each
// texture admission used to copy every payload of its container). I-0074: they are views; a raw TPF is read where it lies.
std::vector<CpuContainerEntryView> ViewContainer(Bytes bytes,std::optional<size_t> only,std::vector<uint8_t>& expanded) {
    expanded.clear();
    if(!Magic(bytes,0,"TPF\0"))expanded=Expand(bytes);else Require(bytes.size()<=Limit,"CPU container input bound");
    const Bytes data=expanded.empty()?bytes:Bytes(expanded);
    Require(data.size()>=16 && Magic(data,0,"TPF\0") && !data[12] && data[13]<=3 && data[14]<=2 && !data[15],"unsupported CPU TPF platform/header");
    const auto data_length=Word(data,4),count=Word(data,8);
    Require(data_length<=data.size() && count>0 && count<=65536,"CPU TPF count/data bound");
    std::vector<Record> records;records.reserve(count);std::set<std::string> names;size_t cursor=16;
    for(uint32_t i=0;i<count;++i) {
        Require(Extent(cursor,20,data.size()),"CPU TPF entry header");
        Record record{Word(data,cursor),Word(data,cursor+4),{},data[cursor+8],data[cursor+9],data[cursor+10],{}};
        const auto flags=data[cursor+11];const auto name_offset=Word(data,cursor+12),floats=Word(data,cursor+16);cursor+=20;
        Require(flags<=1 && record.type<=2 && floats<=1,"unsupported CPU TPF entry compression/type");
        if(floats) {
            Require(Extent(cursor,8,data.size()),"CPU TPF optional metadata header extent");
            const auto length=Word(data,cursor+4);
            Require(!(length&3) && Extent(cursor+8,length,data.size()),"CPU TPF optional metadata extent");
            auto& metadata=record.float_metadata.emplace();metadata.tag=Word(data,cursor);
            metadata.words.reserve(length/4);
            for(size_t word=0;word<length/4;++word)metadata.words.push_back(Word(data,cursor+8+word*4));
            cursor+=8+length;
        }
        record.name=ReadName(data,name_offset,data[14]);
        Require(Extent(record.start,record.length,data.size()) && record.length>=128 &&
            Magic(data,record.start,"DDS "),"CPU TPF DDS extent/signature");
        Require(names.insert(record.name.value).second,"CPU TPF duplicate entry name");records.push_back(std::move(record));
    }
    // Reject aliases with bounded O(n log n) work before exposing values.
    std::vector<std::pair<size_t,size_t>> ranges;size_t payload_bytes{};
    for(const auto& record:records) {
        Require(record.start>=cursor && record.name.begin>=cursor,"CPU TPF header alias");
        ranges.emplace_back(record.start,record.start+record.length);payload_bytes+=record.length;
    }
    std::sort(ranges.begin(),ranges.end());
    for(size_t i=1;i<ranges.size();++i)Require(ranges[i-1].second<=ranges[i].first,"CPU TPF payload alias");
    Require(payload_bytes<=data_length,"CPU TPF declared data too small");
    for(const auto& record:records) {
        const auto next=std::lower_bound(ranges.begin(),ranges.end(),record.name.begin,
            [](const auto& range,size_t start){return range.second<=start;});
        Require(next==ranges.end() || next->first>=record.name.end,"CPU TPF name/payload alias");
    }
    std::vector<CpuContainerEntryView> output;output.reserve(only?1:records.size());
    for(size_t index=0;index<records.size();++index) {
        if(only && *only!=index)continue;auto& record=records[index];
        output.push_back({std::move(record.name.value),record.format,record.type,record.mips,data.subspan(record.start,record.length),std::move(record.float_metadata)});
    }
    return output;
}
std::vector<CpuContainerTexture> DecodeContainer(Bytes bytes,std::optional<size_t> only) {
    std::vector<uint8_t> expanded;auto views=ViewContainer(bytes,only,expanded);
    std::vector<CpuContainerTexture> output;output.reserve(views.size());
    for(auto& view:views)output.push_back({std::move(view.name),view.format_tag,view.type,view.mip_count,{view.dds.begin(),view.dds.end()},std::move(view.float_metadata)});
    return output;
}
}
std::vector<CpuContainerTexture> DecodeCpuTextureContainer(const std::vector<uint8_t>& bytes) {return DecodeContainer(bytes,std::nullopt);}
std::optional<CpuContainerEntryView> ViewCpuTextureContainerEntry(std::span<const uint8_t> bytes,size_t index,std::vector<uint8_t>& expanded) {
    auto entries=ViewContainer(bytes,index,expanded);
    if(entries.empty())return std::nullopt;
    return std::move(entries.front());
}
}
