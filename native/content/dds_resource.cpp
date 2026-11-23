#include "dds_resource.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace ds3content {
namespace {
constexpr size_t Limit=128u*1024u*1024u;
void Require(bool value,const char* message){if(!value)throw std::invalid_argument(message);}
uint32_t Word(std::span<const uint8_t> bytes,size_t offset) {
    Require(offset<=bytes.size() && bytes.size()-offset>=4,"native DDS field extent");
    uint32_t value{};std::memcpy(&value,bytes.data()+offset,4);return value;
}
// I-0063: every block-compressed family and every plain element size (the exported packages hold BC5 hair maps and
// sRGB BGRA textures; an unknown format had refused the texture and, in the host's scan, the rest of its package).
uint32_t BlockBytes(DXGI_FORMAT format) noexcept {
    const auto f=static_cast<uint32_t>(format);
    if((f>=70 && f<=72) || (f>=79 && f<=81))return 8;                        // BC1, BC4
    if((f>=73 && f<=78) || (f>=82 && f<=84) || (f>=94 && f<=99))return 16;   // BC2, BC3, BC5, BC6H, BC7
    return 0;
}
uint32_t PixelBytes(DXGI_FORMAT format) noexcept {
    const auto f=static_cast<uint32_t>(format);
    if(f>=1 && f<=4)return 16;
    if(f>=5 && f<=8)return 12;
    if(f>=9 && f<=18)return 8;
    if((f>=23 && f<=43) || (f>=87 && f<=93))return 4;
    if((f>=48 && f<=59) || f==85 || f==86 || f==115)return 2;
    if((f>=60 && f<=63) || f==65)return 1;
    return 0;
}
bool Shape(uint32_t width,uint32_t height,DXGI_FORMAT format,uint32_t& row_bytes,uint32_t& rows) noexcept {
    if(!width || !height || width>16384 || height>16384)return false;
    if(const auto block=BlockBytes(format)) {
        row_bytes=std::max(1u,(width+3)/4)*block;rows=std::max(1u,(height+3)/4);return true;
    }
    if(const auto pixel=PixelBytes(format)) {row_bytes=width*pixel;rows=height;return true;}
    return false;
}
DXGI_FORMAT LegacyFormat(std::span<const uint8_t> data) {
    const auto flags=Word(data,80),fourcc=Word(data,84);
    if(flags&4) {
        if(fourcc==0x31545844)return DXGI_FORMAT_BC1_UNORM;
        if(fourcc==0x35545844)return DXGI_FORMAT_BC3_UNORM;
        if(fourcc==0x33545844)return DXGI_FORMAT_BC2_UNORM;
        if(fourcc==0x31495441 || fourcc==0x55344342)return DXGI_FORMAT_BC4_UNORM;    // ATI1, BC4U
        if(fourcc==0x32495441 || fourcc==0x55354342)return DXGI_FORMAT_BC5_UNORM;    // ATI2, BC5U (I-0063)
        if(fourcc==0x53344342)return DXGI_FORMAT_BC4_SNORM;
        if(fourcc==0x53354342)return DXGI_FORMAT_BC5_SNORM;
        if(fourcc==111)return DXGI_FORMAT_R16_FLOAT;
        if(fourcc==112)return DXGI_FORMAT_R16G16_FLOAT;
        if(fourcc==113)return DXGI_FORMAT_R16G16B16A16_FLOAT;
        if(fourcc==114)return DXGI_FORMAT_R32_FLOAT;
        if(fourcc==115)return DXGI_FORMAT_R32G32_FLOAT;
        if(fourcc==116)return DXGI_FORMAT_R32G32B32A32_FLOAT;
    } else if((flags&0x40) && Word(data,88)==32 && Word(data,92)==0xFF0000 &&
        Word(data,96)==0xFF00 && Word(data,100)==0xFF) {
        if(!Word(data,104))return DXGI_FORMAT_B8G8R8X8_UNORM;
        if(Word(data,104)==0xFF000000)return DXGI_FORMAT_B8G8R8A8_UNORM;
    } else if((flags&0x40) && Word(data,88)==32 && Word(data,92)==0xFF && Word(data,96)==0xFF00 &&
        Word(data,100)==0xFF0000 && Word(data,104)==0xFF000000) return DXGI_FORMAT_R8G8B8A8_UNORM;   // I-0063: the maps' colour-grading tables (m40_00_CGrading*, 256x16)
    throw std::invalid_argument("unsupported native DDS legacy pixel format");
}
}
CpuTextureResource DecodeNativeTextureDDS(const CpuContainerTexture& input) {
    return DecodeNativeTextureDDS(CpuContainerEntryView{input.name,input.format_tag,input.type,input.mip_count,input.dds,input.float_metadata});
}
CpuTextureResource DecodeNativeTextureDDS(const CpuContainerEntryView& input) {
    const auto data=input.dds;
    Require(!input.name.empty() && input.name.size()<=1024 && input.type<=1 &&
        data.size()>=128 && data.size()<=Limit && !std::memcmp(data.data(),"DDS ",4) &&
        Word(data,4)==124 && Word(data,76)==32,"native DDS header or texture family");
    CpuTextureResource result{};result.name=input.name;result.tpf_format_tag=input.format_tag;
    result.float_metadata=input.float_metadata;
    result.width=Word(data,16);result.height=Word(data,12);result.mip_levels=std::max(1u,Word(data,28));
    const auto caps2=Word(data,112);
    Require(!(caps2&0x200000) && Word(data,24)<=1,"native DDS volume not implemented");
    Require(!(caps2&0xFC00) || (caps2&0x200),"native DDS cube face flags require cube master flag");
    result.cube=(caps2&0x200)!=0;result.array_slices=result.cube?6u:1u;size_t offset=128;
    if((Word(data,80)&4) && Word(data,84)==0x30315844) {
        Require(data.size()>=148 && Word(data,132)==3,"native DDS DX10 dimensions not implemented");
        const auto misc=Word(data,136),array=Word(data,140);
        result.cube=(misc&4)!=0;
        Require(!(caps2&0x200) || result.cube,"native DDS legacy/DX10 cube declarations disagree");
        Require(array>0 && array<=64 && (!result.cube || array<=10),"native DDS array bound");
        result.array_slices=array*(result.cube?6u:1u);
        result.format=static_cast<DXGI_FORMAT>(Word(data,128));offset=148;
    } else {
        if(result.cube)Require((caps2&0xFC00)==0xFC00,"native DDS cube requires all six faces");
        result.format=LegacyFormat(data);
    }
    Require(result.cube==(input.type==1) && (!result.cube || result.width==result.height),"native DDS TPF/cube mismatch");
    Require(result.mip_levels<=15 && result.width && result.height &&
        result.width<=16384 && result.height<=16384,"native DDS dimensions/mips");
    uint32_t max_mips=1;
    for(uint32_t dimension=std::max(result.width,result.height);dimension>1;dimension>>=1)++max_mips;
    Require(result.mip_levels<=max_mips && (!input.mip_count || input.mip_count==result.mip_levels),"native DDS mip metadata");
    if(result.cube && result.array_slices>6 && result.array_slices%36==0) {   // I-0063: a DX10 cube whose array size counts faces (GILM0010: 6 = one cube), when the data holds exactly that many faces
        uint64_t per_slice=0; for(uint32_t mip=0;mip<result.mip_levels;++mip) { uint32_t rb{},rows{}; if(Shape(std::max(1u,result.width>>mip),std::max(1u,result.height>>mip),result.format,rb,rows)) per_slice+=static_cast<uint64_t>(rb)*rows; }
        if(per_slice && offset+per_slice*(result.array_slices/6)==data.size()) result.array_slices/=6; }
    result.subresources.reserve(static_cast<size_t>(result.array_slices)*result.mip_levels);
    for(uint32_t slice=0;slice<result.array_slices;++slice)for(uint32_t mip=0;mip<result.mip_levels;++mip) {
        CpuTextureSubresource sub{};
        sub.width=std::max(1u,result.width>>mip);sub.height=std::max(1u,result.height>>mip);
        Require(Shape(sub.width,sub.height,result.format,sub.row_bytes,sub.rows),"unsupported native DDS allocation format");
        const auto count=static_cast<size_t>(sub.row_bytes)*sub.rows;
        Require(offset<=data.size() && count<=data.size()-offset,"native DDS complete subresource extent");
        sub.bytes.assign(data.begin()+offset,data.begin()+offset+count);offset+=count;
        result.subresources.push_back(std::move(sub));
    }
    Require(offset==data.size() && ValidCpuTextureResource(result),"native DDS trailing data or incomplete resource");return result;
}
size_t NativeLegacyDdsByteCount(const std::vector<uint8_t>& header){
    Require(header.size()==128 && !std::memcmp(header.data(),"DDS ",4) &&
        Word(header,4)==124 && Word(header,76)==32,"native direct DDS header");
    Require(!((Word(header,80)&4) && Word(header,84)==0x30315844),"native direct DDS DX10 extent unknown");
    const auto width=Word(header,16),height=Word(header,12),mips=std::max(1u,Word(header,28)),caps2=Word(header,112);
    Require(!(caps2&0x200000) && Word(header,24)<=1 && width && height &&
        width<=16384 && height<=16384 && mips<=15,"native direct DDS dimensions");
    const bool cube=(caps2&0x200)!=0;
    Require(!(caps2&0xFC00) || cube,"native direct DDS cube declaration");
    Require(!cube || ((caps2&0xFC00)==0xFC00 && width==height),"native direct DDS six cube faces");
    uint32_t max_mips=1;
    for(uint32_t dimension=std::max(width,height);dimension>1;dimension>>=1)++max_mips;
    Require(mips<=max_mips,"native direct DDS mip count");
    const auto format=LegacyFormat(header);uint64_t bytes=128;
    for(uint32_t face=0;face<(cube?6u:1u);++face)for(uint32_t mip=0;mip<mips;++mip){uint32_t row{},rows{};
        Require(Shape(std::max(1u,width>>mip),std::max(1u,height>>mip),format,row,rows),"native direct DDS format");
        const auto count=uint64_t(row)*rows;Require(count<=Limit-bytes,"native direct DDS extent bound");bytes+=count;
    }
    return static_cast<size_t>(bytes);
}
bool ValidCpuTextureResource(const CpuTextureResource& input) noexcept {
    if(input.name.empty() || input.name.size()>1024 || !input.width || !input.height ||
        input.width>16384 || input.height>16384 || !input.mip_levels || input.mip_levels>15 ||
        !input.array_slices || input.array_slices>64 || (input.cube && (input.width!=input.height || input.array_slices%6)) ||
        input.subresources.size()!=static_cast<size_t>(input.array_slices)*input.mip_levels)return false;
    uint32_t max_mips=1;
    for(uint32_t dimension=std::max(input.width,input.height);dimension>1;dimension>>=1)++max_mips;
    if(input.mip_levels>max_mips)return false;
    size_t total{};
    for(size_t index=0;index<input.subresources.size();++index) {
        const auto mip=static_cast<uint32_t>(index%input.mip_levels);
        const auto& sub=input.subresources[index];uint32_t row_bytes{},rows{};
        if(sub.width!=std::max(1u,input.width>>mip) || sub.height!=std::max(1u,input.height>>mip) ||
            !Shape(sub.width,sub.height,input.format,row_bytes,rows) || sub.row_bytes!=row_bytes || sub.rows!=rows ||
            sub.bytes.size()!=static_cast<size_t>(row_bytes)*rows || sub.bytes.size()>Limit-total)return false;
        total+=sub.bytes.size();
    }
    return true;
}
}
