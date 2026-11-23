#include "game_primitive_image_source.h"
#include <array>
#include <cstdio>
#include <stdexcept>
using namespace ds3native;
void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
int main(){try{
    std::array<uint8_t,12> glyph{1,2,3,99,99,99,4,5,6,99,99,99};
    std::array<uint8_t,2> tail{7,99};
    const std::array<bootstrap::CpuPrimitiveImageMip,2> mips{{{3,2,6,glyph},{1,1,2,tail}}};
    frame::PrimitiveTexture image;
    Require(bootstrap::ProduceCpuPrimitiveImage(41,9,mips,image),"glyph rejected");
    Require(image.texture.mips[0]==std::vector<uint8_t>({1,2,3,4,5,6}) && image.texture.mips[1]==std::vector<uint8_t>({7}),"padded glyph rows");
    auto malformed=mips;malformed[1].width=2;
    Require(!bootstrap::ProduceCpuPrimitiveImage(42,9,malformed,image) && image.identity==41,"invalid mip replaced publication");
    Require(!bootstrap::ProduceCpuPrimitiveImage(42,200,mips,image),"video plane admitted");
    malformed=mips;malformed[0].bytes=std::span(glyph).first(11);
    Require(!bootstrap::ProduceCpuPrimitiveImage(42,9,malformed,image),"truncated source accepted");
    glyph.fill(0);tail.fill(0);Require(image.texture.mips[0][5]==6 && image.texture.mips[1][0]==7,"borrowed decoder lifetime");
    // Offset RawImage pointer at a bottom/right crop: the last row's padding
    // is outside the source allocation and must never be read or retained.
    SceneTexture atlas;atlas.width=5;atlas.height=4;atlas.format=DXGI_FORMAT_R8_UNORM;atlas.mips.emplace_back(size_t{20},uint8_t{0});
    frame::PrimitiveTextureStore store;
    Require(store.Define({1,1},{{81,atlas}})==frame::PrimitiveTextureStatus::Ready,"atlas ownership");
    const auto before=store.Acquire({1,1});std::array<uint8_t,7> raster{11,12,99,99,99,21,22};
    std::array<bootstrap::CpuGlyphImageUpdate,1> update{{{{5,4,5,raster},{3,2,5,4},0}}};
    std::vector<frame::PrimitiveTextureEdit> edits;
    Require(bootstrap::ProduceCpuGlyphImageUpdates(81,atlas,update,edits),"offset glyph crop rejected");
    Require(store.Commit({1,1},2,edits)==frame::PrimitiveTextureStatus::Ready,"glyph publication");
    const auto after=store.Acquire({1,2});raster.fill(0);
    Require(after->textures[0].texture.mips[0][13]==11 && after->textures[0].texture.mips[0][19]==22 &&
        after->textures[0].texture.mips[0][12]==0 && before->textures[0].texture.mips[0][19]==0,"glyph region/current and retained generations");
    auto invalid=update;invalid[0].destination[2]=6;
    Require(!bootstrap::ProduceCpuGlyphImageUpdates(81,atlas,invalid,edits),"atlas overrun admitted");
    invalid=update;invalid[0].source.bytes=std::span(raster).first(6);
    Require(!bootstrap::ProduceCpuGlyphImageUpdates(81,atlas,invalid,edits),"short final glyph row admitted");
    Require(store.Commit({1,1},3,edits)==frame::PrimitiveTextureStatus::Stale && store.Acquire({1,2})==after,"stale atlas update");
    Require(store.Withdraw({1,2}) && after->textures[0].texture.mips[0][19]==22,"atlas owner retirement");
    for(const auto [source,format]:std::array<std::pair<uint32_t,DXGI_FORMAT>,6>{{{1,DXGI_FORMAT_R8G8B8A8_UNORM},{2,DXGI_FORMAT_B8G8R8A8_UNORM},{50,DXGI_FORMAT_BC1_UNORM},{51,DXGI_FORMAT_BC2_UNORM},{52,DXGI_FORMAT_BC3_UNORM},{53,DXGI_FORMAT_BC7_UNORM}}}){
        frame::PrimitiveTextureLayout layout;Require(frame::GetPrimitiveTextureLayout(7,5,format,layout),"physical format absent");
        std::vector<uint8_t> bytes(size_t(layout.row_bytes+5)*layout.rows,0xAD);
        const std::array<bootstrap::CpuPrimitiveImageMip,1> input{{{7,5,layout.row_bytes+5,bytes}}};
        Require(bootstrap::ProduceCpuPrimitiveImage(43,source,input,image) && image.texture.format==format &&
            image.texture.mips[0].size()==size_t(layout.row_bytes)*layout.rows,"odd block extent or native format changed");
    }
    std::puts("PASS CPU primitive images: seven formats, padded rows/explicit mips/odd extents, atomic refusal, owned decoder lifetime; cropped glyph updates/current generations/stale refusal/retained atlas retirement");return 0;
}catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}}
