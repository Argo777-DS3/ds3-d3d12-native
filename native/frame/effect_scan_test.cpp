// I-0113 tranche 29: the non-finite scan of a half-float picture (renderer/effect_scan.h) on hand-made pixels: NaN, +inf, -inf,
// negatives, -0, the largest finite half and subnormals, their channels and the boxes, a row pitch wider than the row.
#include "../renderer/effect_scan.h"
#include <cstdio>
#include <vector>

using namespace ds3native::effect_scan;
namespace {
int failures{},checks{};
void Expect(const char* what,uint32_t got,uint32_t expected){++checks;if(got==expected)return;++failures;std::printf("FAIL %s: %u, expected %u\n",what,got,expected);}
void Box(const char* what,const std::array<uint32_t,4>& got,std::array<uint32_t,4> expected){
    for(int i=0;i<4;++i){char w[96];std::snprintf(w,sizeof w,"%s[%d]",what,i);Expect(w,got[i],expected[i]);}}
}
int main() {
    constexpr uint32_t w=7,h=5,pitch_pixels=9;   // a pitch of 9 pixels: the two pixels past each row are never read
    std::vector<uint16_t> picture(size_t(pitch_pixels)*h*4,0x3C00);   // 1.0 everywhere
    const auto at=[&](uint32_t x,uint32_t y,int c)->uint16_t&{return picture[(size_t(y)*pitch_pixels+x)*4+c];};
    for(uint32_t y=0;y<h;++y)for(uint32_t c=0;c<4;++c){at(7,y,c)=0x7E00;at(8,y,c)=0xBC00;}   // padding: NaN and -1, not to be counted
    at(0,0,0)=0x8000;    // -0: not negative
    at(1,0,1)=0x7BFF;    // 65504, the largest finite half
    at(2,0,2)=0x0001;    // the smallest subnormal
    at(3,0,3)=0x8001;    // a negative subnormal: negative
    at(2,1,1)=0x7E00;    // NaN in G
    at(5,3,0)=0x7C01;    // NaN in R (the smallest payload)
    at(4,2,2)=0x7C00;    // +inf in B
    at(1,4,0)=0xFC00;    // -inf in R: inf, not negative
    at(6,1,2)=0xBC00;    // -1 in B
    at(0,3,1)=0xC000;    // -2 in G
    const Counts c=Scan(reinterpret_cast<const uint8_t*>(picture.data()),uint64_t(pitch_pixels)*8,w,h);
    Expect("nan R",c.nan[0],1);Expect("nan G",c.nan[1],1);Expect("nan B",c.nan[2],0);Expect("nan A",c.nan[3],0);
    Expect("inf R",c.inf[0],1);Expect("inf B",c.inf[2],1);Expect("inf G",c.inf[1],0);
    Expect("neg R",c.neg[0],0);Expect("neg G",c.neg[1],1);Expect("neg B",c.neg[2],1);Expect("neg A",c.neg[3],1);
    Box("bad box",c.bad_box,{1,1,6,5});   // (2,1), (5,3), (4,2), (1,4)
    Box("negative box",c.neg_box,{0,0,7,4});   // (3,0), (6,1), (0,3)
    ++checks;if(!c.Any()){++failures;std::puts("FAIL Any");}
    // A clean picture: nothing, empty boxes.
    std::vector<uint16_t> clean(size_t(w)*h*4,0x3C00);clean[5]=0x8000;clean[9]=0x7BFF;
    const Counts z=Scan(reinterpret_cast<const uint8_t*>(clean.data()),uint64_t(w)*8,w,h);
    ++checks;if(z.Any()){++failures;std::puts("FAIL a clean picture flagged");}
    Box("clean bad box",z.bad_box,{0,0,0,0});Box("clean negative box",z.neg_box,{0,0,0,0});
    // Every exponent-all-ones lane in each position of a pixel, the others finite: each found in its channel.
    for(int lane=0;lane<4;++lane){std::vector<uint16_t> one(4,0x3C00);one[lane]=0x7C00;
        const Counts o=Scan(reinterpret_cast<const uint8_t*>(one.data()),8,1,1);char what[32];std::snprintf(what,sizeof what,"lane %d inf",lane);Expect(what,o.inf[lane],1);}
    // The step review's L3: a negative NaN (0xFE00, 0xFC01) is a NaN, not a negative; a sign in one lane beside an all-ones exponent
    // in another lane of the same pixel: each counted in its own channel, the pixel in both boxes.
    {const uint16_t mixed[8]{0xFE00,0xFC01,0x3C00,0x3C00, 0xBC00,0x3C00,0x7C00,0x3C00};
     const Counts m=Scan(reinterpret_cast<const uint8_t*>(mixed),16,2,1);
     Expect("negative NaN R",m.nan[0],1);Expect("negative NaN G",m.nan[1],1);   // R 0xFE00, G 0xFC01: each one NaN
     Expect("negative R (the -1 only)",m.neg[0],1);Expect("negative G (none)",m.neg[1],0);Expect("negatives B, A",m.neg[2]+m.neg[3],0);Expect("mixed lane inf B",m.inf[2],1);
     Box("mixed bad box",m.bad_box,{0,0,2,1});Box("mixed negative box",m.neg_box,{1,0,2,1});}
    std::printf("effect scan: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
