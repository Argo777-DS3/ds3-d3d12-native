#pragma once
// I-0113 tranche 29 (effect-scan.flag, gap 16): the non-finite and negative half floats of an R16G16B16A16_FLOAT picture, by
// channel, and the boxes of the pixels that hold them. STUDY-GAPS-16-13 §3: the 608 radial blur keeps sqrt(mean(tap^2)) per
// channel then max(..., 0), so a NaN in one channel of its source kills that channel over its disc (green killed: magenta, red
// killed: cyan); this scan says whether the native's source or output holds such values. Diagnostic only (behind its flag).
#include <array>
#include <cstdint>
#include <cstring>

namespace ds3native::effect_scan {
struct Counts {
    std::array<uint32_t,4> nan{},inf{},neg{};   // R, G, B, A: NaN, +-inf, negative (sign set, not -0)
    std::array<uint32_t,4> bad_box{},neg_box{};  // x0, y0, x1, y1 (exclusive): the pixels with a NaN or inf, those with a negative; {0,0,0,0} none
    bool Any() const noexcept {for(int c=0;c<4;++c)if(nan[c] || inf[c] || neg[c])return true;return false;}
};
// A box grown to hold (x, y).
inline void Grow(std::array<uint32_t,4>& box,uint32_t x,uint32_t y) noexcept {
    if(box[2]==0){box={x,y,x+1,y+1};return;}
    if(x<box[0])box[0]=x;if(y<box[1])box[1]=y;if(x+1>box[2])box[2]=x+1;if(y+1>box[3])box[3]=y+1;}
// The rows of `height` rows of `width` pixels (8 bytes each), `pitch` bytes apart, from `base`. A pixel whose four halves have
// a finite exponent and a clear sign (the common case) is passed over with one test of its 64 bits.
inline Counts Scan(const uint8_t* base,uint64_t pitch,uint32_t width,uint32_t height) noexcept {
    constexpr uint64_t exponents=0x7C007C007C007C00ull,signs=0x8000800080008000ull;
    Counts out;
    for(uint32_t y=0;y<height;++y){const uint8_t* row=base+pitch*y;
        for(uint32_t x=0;x<width;++x){uint64_t p;std::memcpy(&p,row+uint64_t(x)*8,8);
            // A lane whose exponent is all ones: (p & exponents) ^ exponents has a zero 16-bit lane there.
            const uint64_t t=(p&exponents)^exponents;
            const bool special=((t-0x0001000100010001ull)&~t&signs)!=0;
            if(!special && !(p&signs))continue;
            bool bad=false,negative=false;
            for(int c=0;c<4;++c){const uint16_t h=uint16_t(p>>(16*c));
                if((h&0x7C00)==0x7C00){bad=true;if(h&0x03FF)++out.nan[c];else ++out.inf[c];}
                else if((h&0x8000) && (h&0x7FFF)){negative=true;++out.neg[c];}}
            if(bad)Grow(out.bad_box,x,y);
            if(negative)Grow(out.neg_box,x,y);}}
    return out;
}
}
