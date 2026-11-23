// I-0135 (final review L5): the Bandai Namco logo made for black (dark_logo.h) on a synthetic BC1 image, 48x48 in solid 4x4
// blocks: a white ground, an orange blob holding a white letter whose counter is orange and two peach blocks (no zero channel:
// one at its edge, one inside), a red letter on the white whose counter is white; BC1's four- and three-colour blocks; and the
// logo file's parser (ParseLogoDds) with its refusals.
#include "dark_logo.h"
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace ds3native::frame;
namespace {
void Require(bool v,const char* m){if(!v)throw std::runtime_error(m);}
constexpr uint32_t size=48,blocks=size/4;
constexpr uint16_t white=0xFFFF,orange=(31<<11)|(23<<5),red=(28<<11)|2,peach=(31<<11)|(44<<5)|14;   // peach: no zero channel
uint16_t BlockColour(uint32_t bx,uint32_t by){
    if(bx>=2 && bx<=4 && by>=2 && by<=4)return bx==3 && by==3?orange:white;   // the white letter, its orange counter
    if((bx==6 || bx==5) && by==3)return peach;                                 // on the blob: at its edge, and inside
    if(bx>=1 && bx<=6 && by>=1 && by<=5)return orange;                        // the blob
    if(bx>=8 && bx<=10 && by>=7 && by<=9)return bx==9 && by==8?white:red;      // the red letter, its white counter
    return white;
}
std::vector<uint8_t> Image(){
    std::vector<uint8_t> bc1;
    for(uint32_t by=0;by<blocks;++by)for(uint32_t bx=0;bx<blocks;++bx){const auto c=BlockColour(bx,by);
        bc1.insert(bc1.end(),{uint8_t(c),uint8_t(c>>8),uint8_t(c),uint8_t(c>>8),0,0,0,0});}   // every texel index 0: the colour c0
    return bc1;
}
const uint8_t* At(const std::vector<uint8_t>& rgba,uint32_t bx,uint32_t by){return &rgba[((by*4+1)*size+bx*4+1)*4];}   // a block's inner texel
bool Same(const uint8_t* a,const uint8_t* b){return a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3];}
std::vector<uint8_t> Dds(uint32_t w,uint32_t h){
    std::vector<uint8_t> f(128+size_t(w)*h*4,7);
    const auto put=[&](size_t at,uint32_t v){for(int k=0;k<4;++k)f[at+k]=uint8_t(v>>(8*k));};
    put(0,0x20534444);put(4,124);put(12,h);put(16,w);put(76,32);put(80,0x41);put(88,32);put(92,0xFF);put(96,0xFF00);put(100,0xFF0000);put(104,0xFF000000);
    return f;
}
}
int main(){try{
    const auto bc1=Image();std::vector<uint8_t> source,made;
    Require(DecodeBc1(size,size,bc1,source),"I-0135: the synthetic image decodes");
    Require(DarkLogoImage(size,size,bc1,made) && made.size()==source.size(),"I-0135: the logo for black is made");
    Require(At(made,0,0)[3]==0 && At(made,11,11)[3]==0 && At(made,7,3)[3]==0,"I-0135: the white ground transparent");
    const uint8_t opaque_white[4]{255,255,255,255};
    Require(Same(At(made,2,2),opaque_white) && Same(At(made,4,4),opaque_white),"I-0135: the white letter on the blob kept");
    Require(Same(At(made,3,3),At(source,3,3)),"I-0135: the letter's orange counter untouched");
    Require(Same(At(made,5,3),At(source,5,3)),"I-0135: the blob's interior untouched");
    Require(Same(At(made,8,7),At(source,8,7)) && At(made,10,9)[3]==255,"I-0135: the red letter opaque, its colour kept");
    Require(At(made,9,8)[3]==0,"I-0135: the red letter's white counter transparent");
    Require(At(made,6,3)[3]<255 && At(made,6,3)[3]>0 && Same(At(made,5,3),At(source,5,3)),
        "I-0135: the blob's edge band unmixed from white, its interior of the same colour untouched");
    {// BC1's other paths: four colours (c0 > c1, every index) and three with the transparent texel (c0 <= c1, index 3).
        const std::vector<uint8_t> two{0xFF,0xFF,0x00,0x00,0xE4,0,0,0, 0x00,0x00,0xFF,0xFF,0x0B,0,0,0};std::vector<uint8_t> px;
        Require(DecodeBc1(8,4,two,px),"I-0135: two blocks decode");
        const auto texel=[&](uint32_t x,uint32_t y){return &px[(y*8+x)*4];};
        const uint8_t w[4]{255,255,255,255},k[4]{0,0,0,255},third[4]{170,170,170,255},two_thirds[4]{85,85,85,255},half[4]{127,127,127,255},none[4]{0,0,0,0};
        Require(Same(texel(0,0),w) && Same(texel(1,0),k) && Same(texel(2,0),third) && Same(texel(3,0),two_thirds),"I-0135: BC1 four colours");
        Require(Same(texel(4,0),none) && Same(texel(5,0),half) && Same(texel(6,0),k),"I-0135: BC1 three colours and the transparent texel");}
    Require(!DarkLogoImage(size+2,size,bc1,made) && !DarkLogoImage(size,size,std::vector<uint8_t>(bc1.begin(),bc1.end()-8),made),
        "I-0135: a wrong size and a short block array refused");
    std::vector<uint8_t> pixels;const auto file=Dds(size,size);
    Require(ParseLogoDds(file,size,pixels) && pixels.size()==size_t(size)*size*4 && pixels[0]==7,"I-0135: the logo file read");
    auto truncated=file;truncated.pop_back();
    auto compressed=file;compressed[80]|=4;
    auto magic=file;magic[0]='X';
    auto masks=file;masks[92]=0;
    Require(!ParseLogoDds(truncated,size,pixels) && !ParseLogoDds(file,size*2,pixels) && !ParseLogoDds(compressed,size,pixels) &&
        !ParseLogoDds(magic,size,pixels) && !ParseLogoDds(masks,size,pixels) && !ParseLogoDds({},size,pixels),"I-0135: other files refused");
    std::printf("PASS dark logo: ground transparent, the blob's letter and its counter kept, the red letter kept and its counter "
        "transparent, the edge band unmixed and the interior untouched, BC1 four and three colours, sizes refused; the logo file read, five other files refused\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
