// I-0113 tranche 22 (gap 21): FF630's ordered-dither pair, the model programmes' g_DitherTexture (t38): 4x4 A8_UNORM, one mip,
// the bytes FF630's 16 ints give (v * 255 / 15, VERIFIED on 140100530..14010056F), the second their inverse; and the coverage rule
// the `_[A]` programmes' discard makes of them ((d - 0.5) * 0.999 + fade < 0.5 drops a pixel, d its texel at SV_Position x 0.25 with
// a wrap sampler: one texel a pixel): a fade of k / 16 keeps k of a cell's 16 pixels, and a LOD crossfade's two rows, one on each
// texture, with fades a and 1 - a, cover each pixel once.
#include "../content/model_common_textures.h"
#include <cstdio>
#include <stdexcept>

namespace {void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
bool Kept(uint8_t texel,float fade){return !((texel/255.f-0.5f)*0.999f+fade<0.5f);}}
int main(){try{
    const auto pair=ds3content::ProduceModelFadeDitherTextures();
    constexpr uint8_t first[16]{0x00,0x88,0x22,0xAA, 0xCC,0x44,0xEE,0x66, 0x33,0xBB,0x11,0x99, 0xFF,0x77,0xDD,0x55};
    for(size_t index=0;index<2;++index){const auto& image=pair[index];
        Require(image.width==4 && image.height==4 && image.mip_levels==1 && image.array_slices==1 && image.format==DXGI_FORMAT_A8_UNORM && !image.cube,"4x4 A8_UNORM, one mip");
        Require(image.subresources.size()==1 && image.subresources[0].row_bytes==4 && image.subresources[0].rows==4 && image.subresources[0].bytes.size()==16,"one 4x4 plane of 4-byte rows");
        for(size_t i=0;i<16;++i)Require(image.subresources[0].bytes[i]==(index?0xFF-first[i]:first[i]),index?"the inverse texels":"FF630's texels");}
    // Coverage: every value once (an ordered dither), so a fade's kept count is monotonic, none at 0 and all at 1.
    int previous=-1;
    for(int step=0;step<=16;++step){const float fade=step/16.f;int kept{},both{},none{};
        for(size_t i=0;i<16;++i){const bool a=Kept(pair[0].subresources[0].bytes[i],fade),b=Kept(pair[1].subresources[0].bytes[i],1.f-fade);
            kept+=a;both+=a && b;none+=!a && !b;}
        Require(kept>=previous && kept==step,"a fade of k / 16 keeps k pixels of the cell");previous=kept;
        Require(!both && !none,"a crossfade's two rows cover each pixel once");}
    std::printf("PASS fade dither: FF630's 4x4 A8 pair and its inverse, the kept pixels monotonic in the fade, a crossfade covered once\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
