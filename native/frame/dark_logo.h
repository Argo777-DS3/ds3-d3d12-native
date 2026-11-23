#pragma once
// D-0182 (Thomas, 2026-11-12 02:48: the Bandai Namco screen on black, as in Elden Ring; I-0135). The page 05_901_Logo_BNE
// multiplies the image MENU_BNE_Logo (768x768 BC1, opaque, its blob on white) over a white full-screen rectangle
// (05_901_logo_bne.gfx, read statically, I-0135 RESULT). DarkLogoImage makes, from the game's own image, the same logo for a
// black background, in straight alpha (RGBA8, colour unmixed from white): the white around the blob and the counters of the
// red letters made transparent, the white letters BANDAI NAMCO and the blob kept as they are. The rule, the coordinator's
// (2026-11-12 05:19), is the one `investigations/I-0135-bne-logo-dark/tools/logo_dark_preview.py` applies offline, byte for byte:
//   1. white: min(R, G, B) >= 245; the non-white pixels labelled (4-connected); the blob, the largest component.
//   2. a white component is kept (a letter on the blob) when it does not touch the image's border and touches the blob;
//      every other one (the background, the counters of « Entertainment » and of the trade mark) is background.
//   3. colour-to-alpha from white (alpha = max(255 - c), colour unmixed) on the background, on the non-blob components that
//      touch it (the red letters: identity on their saturated core) and on the blob's pixels within 3 pixels of it; the
//      blob's interior, the kept letters and their counters (non-blob components enclosed by a kept letter) untouched.
#include <algorithm>
#include <cstdint>
#include <vector>

namespace ds3native::frame {
// BC1 (DXT1) blocks to RGBA8, rows top-down; the punch-through index of a block with c0 <= c1 is (0, 0, 0, 0).
inline bool DecodeBc1(uint32_t width,uint32_t height,const std::vector<uint8_t>& blocks,std::vector<uint8_t>& rgba){
    if(!width || !height || width%4 || height%4 || blocks.size()!=size_t(width/4)*(height/4)*8)return false;
    rgba.assign(size_t(width)*height*4,0);
    const auto expand=[](uint32_t c,uint8_t* out){out[0]=uint8_t(((c>>11)&31)*255/31);out[1]=uint8_t(((c>>5)&63)*255/63);out[2]=uint8_t((c&31)*255/31);out[3]=255;};
    size_t pos=0;
    for(uint32_t by=0;by<height/4;++by)for(uint32_t bx=0;bx<width/4;++bx,pos+=8){
        const uint32_t c0=blocks[pos]|blocks[pos+1]<<8,c1=blocks[pos+2]|blocks[pos+3]<<8;
        const uint32_t bits=blocks[pos+4]|blocks[pos+5]<<8|blocks[pos+6]<<16|uint32_t(blocks[pos+7])<<24;
        uint8_t pal[4][4]{};expand(c0,pal[0]);expand(c1,pal[1]);
        for(int k=0;k<3;++k){
            if(c0>c1){pal[2][k]=uint8_t((2*pal[0][k]+pal[1][k])/3);pal[3][k]=uint8_t((pal[0][k]+2*pal[1][k])/3);}
            else pal[2][k]=uint8_t((pal[0][k]+pal[1][k])/2);}
        pal[2][3]=255;pal[3][3]=c0>c1?255:0;
        for(uint32_t i=0;i<16;++i){const auto* p=pal[(bits>>(2*i))&3];
            std::copy(p,p+4,&rgba[((size_t(by)*4+i/4)*width+bx*4+i%4)*4]);}
    }
    return true;
}
// The logo for a black background (see the header); false when the image is not BC1 of that shape or has no blob.
inline bool DarkLogoImage(uint32_t width,uint32_t height,const std::vector<uint8_t>& bc1,std::vector<uint8_t>& out){
    std::vector<uint8_t> px;if(!DecodeBc1(width,height,bc1,px))return false;
    const size_t w=width,h=height,n=w*h;
    std::vector<uint8_t> white(n);
    for(size_t i=0;i<n;++i)white[i]=(std::min)({px[i*4],px[i*4+1],px[i*4+2]})>=245;
    // Labels in scan order, each component filled from its first pixel (the preview's order).
    const auto label=[&](bool member_white,std::vector<uint32_t>& lab,std::vector<size_t>& sizes){
        lab.assign(n,0);sizes.assign(1,0);std::vector<size_t> stack;
        for(size_t start=0;start<n;++start){if(bool(white[start])!=member_white || lab[start])continue;
            const auto id=uint32_t(sizes.size());sizes.push_back(0);lab[start]=id;stack.push_back(start);
            while(!stack.empty()){const auto i=stack.back();stack.pop_back();++sizes[id];const auto x=i%w,y=i/w;
                const auto visit=[&](size_t j){if(bool(white[j])==member_white && !lab[j]){lab[j]=id;stack.push_back(j);}};
                if(x>0)visit(i-1);if(x<w-1)visit(i+1);if(y>0)visit(i-w);if(y<h-1)visit(i+w);}}};
    std::vector<uint32_t> dark,light;std::vector<size_t> dark_sizes,light_sizes;
    label(false,dark,dark_sizes);label(true,light,light_sizes);
    if(dark_sizes.size()<2)return false;
    uint32_t blob=1;for(uint32_t id=2;id<dark_sizes.size();++id)if(dark_sizes[id]>dark_sizes[blob])blob=id;
    std::vector<uint8_t> border(light_sizes.size()),near_blob(light_sizes.size());
    const auto neighbours=[&](size_t i,auto&& f){const auto x=i%w,y=i/w;if(x>0)f(i-1);if(x<w-1)f(i+1);if(y>0)f(i-w);if(y<h-1)f(i+w);};
    for(size_t i=0;i<n;++i){const auto id=light[i];if(!id)continue;const auto x=i%w,y=i/w;
        if(!x || x==w-1 || !y || y==h-1)border[id]=1;
        neighbours(i,[&](size_t j){if(dark[j]==blob)near_blob[id]=1;});}
    std::vector<uint8_t> background(n);
    for(size_t i=0;i<n;++i){const auto id=light[i];background[i]=id && !(!border[id] && near_blob[id]);}
    std::vector<uint8_t> outside(dark_sizes.size());
    for(size_t i=0;i<n;++i){const auto id=dark[i];if(!id || id==blob || outside[id])continue;
        neighbours(i,[&](size_t j){if(background[j])outside[id]=1;});}
    // Within 3 pixels of the background (a 7x7 square), rows then columns through prefix sums.
    constexpr size_t r=3;std::vector<uint8_t> rows(n),within(n);std::vector<uint32_t> acc((std::max)(w,h)+1);
    for(size_t y=0;y<h;++y){for(size_t x=0;x<w;++x)acc[x+1]=acc[x]+background[y*w+x];
        for(size_t x=0;x<w;++x)rows[y*w+x]=acc[(std::min)(w,x+r+1)]-acc[x>r?x-r:0]>0;}
    for(size_t x=0;x<w;++x){for(size_t y=0;y<h;++y)acc[y+1]=acc[y]+rows[y*w+x];
        for(size_t y=0;y<h;++y)within[y*w+x]=acc[(std::min)(h,y+r+1)]-acc[y>r?y-r:0]>0;}
    out=px;
    for(size_t i=0;i<n;++i){
        if(!(background[i] || outside[dark[i]] || (dark[i]==blob && within[i])))continue;
        uint8_t* p=&out[i*4];const int a=255-(std::min)({p[0],p[1],p[2]});
        if(!a){p[0]=p[1]=p[2]=p[3]=0;continue;}
        for(int k=0;k<3;++k)p[k]=uint8_t((std::max)(0,255-((255-p[k])*255+a/2)/a));
        p[3]=uint8_t(a);
    }
    return true;
}
// Thomas, 2026-11-12 05:40 (I-0135 CONTRACT, his PC only): the logo as Elden Ring shows it, written beside the module by the
// local script investigations/I-0135-bne-logo-dark/tools/Extract-EldenRingLogo.ps1 from his own Elden Ring: an uncompressed
// DDS, `size` x `size`, RGBA8 in byte order R, G, B, A (R8G8B8A8_UNORM), one mip. Its pixels, or false for any other file.
inline bool ParseLogoDds(const std::vector<uint8_t>& file,uint32_t size,std::vector<uint8_t>& rgba){
    const auto word=[&](size_t at){return uint32_t(file[at])|uint32_t(file[at+1])<<8|uint32_t(file[at+2])<<16|uint32_t(file[at+3])<<24;};
    if(file.size()!=128+size_t(size)*size*4 || word(0)!=0x20534444u || word(4)!=124 || word(12)!=size || word(16)!=size ||
       word(76)!=32 || (word(80)&0x45)!=0x41 || word(88)!=32 || word(92)!=0xFFu || word(96)!=0xFF00u || word(100)!=0xFF0000u ||
       word(104)!=0xFF000000u)return false;
    rgba.assign(file.begin()+128,file.end());return true;
}
}  // namespace ds3native::frame
