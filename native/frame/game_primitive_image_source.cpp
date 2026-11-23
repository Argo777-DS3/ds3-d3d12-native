#include "game_primitive_image_source.h"
#include <algorithm>
#include <cstring>

namespace ds3native::bootstrap {
bool ProduceCpuPrimitiveImage(uint64_t identity,uint32_t engine_format,
    std::span<const CpuPrimitiveImageMip> mips,frame::PrimitiveTexture& output) {
    DXGI_FORMAT format{};
    // Exact single-plane rows from the installed CPU format table46F8EB0.
    // RGB/palette/video families need their actual CPU conversion first.
    switch(engine_format&0xFFF) {
    case 1:format=DXGI_FORMAT_R8G8B8A8_UNORM;break;
    case 2:format=DXGI_FORMAT_B8G8R8A8_UNORM;break;
    case 9:format=DXGI_FORMAT_R8_UNORM;break;
    case 50:format=DXGI_FORMAT_BC1_UNORM;break;
    case 51:format=DXGI_FORMAT_BC2_UNORM;break;
    case 52:format=DXGI_FORMAT_BC3_UNORM;break;
    case 53:format=DXGI_FORMAT_BC7_UNORM;break;
    default:return false;
    }
    if(!identity || mips.empty() || mips.size()>14)return false;
    frame::PrimitiveTexture candidate;candidate.identity=identity;
    candidate.texture.width=mips[0].width;candidate.texture.height=mips[0].height;candidate.texture.format=format;
    uint64_t total{};
    for(size_t index=0;index<mips.size();++index) {
        const auto& mip=mips[index];frame::PrimitiveTextureLayout layout{};
        if(mip.width!=std::max(1u,mips[0].width>>index) || mip.height!=std::max(1u,mips[0].height>>index) ||
            !frame::GetPrimitiveTextureLayout(mip.width,mip.height,format,layout) || mip.row_pitch<layout.row_bytes ||
            mip.row_pitch>(256u<<20) || uint64_t(layout.rows)*mip.row_pitch!=mip.bytes.size())return false;
        const auto bytes=uint64_t(layout.row_bytes)*layout.rows;
        if(bytes>(256u<<20)-total)return false;
        total+=bytes;
        std::vector<uint8_t> owned(static_cast<size_t>(bytes));
        for(uint32_t row=0;row<layout.rows;++row)std::memcpy(owned.data()+size_t(row)*layout.row_bytes,
            mip.bytes.data()+size_t(row*mip.row_pitch),layout.row_bytes);
        candidate.texture.mips.push_back(std::move(owned));
    }
    if(!frame::ValidPrimitiveTexturePayload(candidate.texture))return false;
    output=std::move(candidate);return true;
}
bool ProduceCpuGlyphImageUpdates(uint64_t identity,const SceneTexture& current,
    std::span<const CpuGlyphImageUpdate> updates,std::vector<frame::PrimitiveTextureEdit>& output) {
    if(!identity || current.format!=DXGI_FORMAT_R8_UNORM || !frame::ValidPrimitiveTexturePayload(current) ||
        updates.empty() || updates.size()>4096)return false;
    std::vector<frame::PrimitiveTextureEdit> candidate;candidate.reserve(updates.size());uint64_t total{};
    for(const auto& update:updates) {
        const auto& r=update.destination;const auto& source=update.source;
        if(update.mip>=current.mips.size() || r[0]<0 || r[1]<0 || r[2]<=r[0] || r[3]<=r[1])return false;
        const auto x=static_cast<uint32_t>(r[0]),y=static_cast<uint32_t>(r[1]);
        const auto width=static_cast<uint32_t>(r[2]-r[0]),height=static_cast<uint32_t>(r[3]-r[1]);
        const auto target_width=std::max(1u,current.width>>update.mip),target_height=std::max(1u,current.height>>update.mip);
        if(x>target_width || width>target_width-x || y>target_height || height>target_height-y ||
            source.width<width || source.height<height || source.width>8192 || source.height>8192 ||
            source.row_pitch<source.width || source.row_pitch>(256u<<20))return false;
        const auto extent=uint64_t(height-1)*source.row_pitch+width,bytes=uint64_t(width)*height;
        if(extent>source.bytes.size() || bytes>(256u<<20)-total)return false;
        total+=bytes;
        frame::PrimitiveTextureWrite patch{identity,update.mip,x,y,width,height,width,std::vector<uint8_t>(static_cast<size_t>(bytes))};
        for(uint32_t row=0;row<height;++row)std::memcpy(patch.bytes.data()+size_t(row)*width,
            source.bytes.data()+size_t(row*source.row_pitch),width);
        candidate.emplace_back(std::move(patch));
    }
    output=std::move(candidate);return true;
}
}
