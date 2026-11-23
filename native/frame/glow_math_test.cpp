// W5d GL-1 T1 (GLOW-WAVE1-READY 7): the glow constant images. The tap bit images against the executable's .rdata (argument:
// DarkSoulsIII.exe; skipped with a message without it), the normalised weights against the table of 3.1, the scaled offsets,
// cbGlowParam for factors -1, 0, 0.5, 1, 2, cbFilterParam for 320 x 180, the level sizes at 1280 x 720, 1920 x 1080 and odd sizes.
#include "../renderer/glow_math.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

using namespace ds3native;
namespace {
int failures{},checks{};
void Expect(bool ok,const std::string& what) {++checks;if(!ok){++failures;std::printf("FAIL %s\n",what.c_str());}}
float Word(const uint8_t* p,size_t i) {float f;std::memcpy(&f,p+4*i,4);return f;}
uint32_t Bits(const uint8_t* p,size_t i) {uint32_t u;std::memcpy(&u,p+4*i,4);return u;}
bool Rdata(const std::vector<uint8_t>& pe,uint32_t rva,void* out,size_t bytes) {
    if(pe.size()<0x400)return false;int32_t nt{};std::memcpy(&nt,pe.data()+0x3C,4);uint16_t count{},optional{};
    std::memcpy(&count,pe.data()+nt+6,2);std::memcpy(&optional,pe.data()+nt+20,2);const size_t table=nt+24+optional;
    for(uint16_t s=0;s<count;++s){uint32_t va{},size{},raw{};const uint8_t* h=pe.data()+table+40*s;
        std::memcpy(&va,h+12,4);std::memcpy(&size,h+16,4);std::memcpy(&raw,h+20,4);
        if(rva>=va && rva-va+bytes<=size){std::memcpy(out,pe.data()+raw+(rva-va),bytes);return true;}}
    return false;
}
}
int main(int argc,char** argv) {
    if(argc>=2){std::ifstream in(argv[1],std::ios::binary);std::vector<uint8_t> pe((std::istreambuf_iterator<char>(in)),{});
        const uint32_t rows[6]{0x3D7FB10,0x3D7F7D0,0x3D80250,0x3D7FA10,0x3D7FA20,0x3D80210};uint32_t words[26]{};bool read=true;
        for(int i=0;i<6;++i)read=read && Rdata(pe,rows[i],words+4*i,16);
        words[24]=0xBF000000;read=read && Rdata(pe,0x3D7F0BC,&words[25],4);uint32_t k{},n{};read=read && Rdata(pe,0x3D7F0C0,&k,4) && Rdata(pe,0x3D7EEFC,&n,4);
        Expect(read,"executable .rdata readable");
        if(read){for(int i=0;i<26;++i)Expect(words[i]==glow::tap_bits[i],"tap word "+std::to_string(i));
            Expect(k==glow::k_bits && n==glow::two_pi_bits,"k and 2 pi words");}}
    else std::printf("glow_math_test: no executable given, the .rdata comparison is skipped\n");
    const double table[13]{0.098182,0.067479,0.067479,0.059550,0.059550,0.067480,0.067480,0.084271,0.087857,0.084271,0.084271,0.087857,0.084271};
    for(const float scale:{1.0f,2.0f,20.0f}){uint8_t image[256]{};glow::TapImage(scale,image);double sum=0;
        for(int i=0;i<13;++i){float x,y;std::memcpy(&x,&glow::tap_bits[2*i],4);std::memcpy(&y,&glow::tap_bits[2*i+1],4);
            Expect(Word(image,4*i)==x*scale && Word(image,4*i+1)==y*scale && Bits(image,4*i+3)==0,"offsets of tap "+std::to_string(i)+" at scale "+std::to_string(scale));
            Expect(std::fabs(Word(image,4*i+2)-table[i])<5e-6,"weight of tap "+std::to_string(i));sum+=Word(image,4*i+2);}
        Expect(std::fabs(sum-1.0)<2e-6,"weights sum to 1");
        for(int i=52;i<64;++i)Expect(Bits(image,i)==0,"entries 13..15 zero");}
    for(const float g:{1.5f,0.5f})for(const float factor:{-1.0f,0.0f,0.5f,1.0f,2.0f}){uint8_t p[16]{};glow::ParamImage(g,0x3F800000,factor,p);
        const float s=factor>1.0f?0.0f:factor<0.0f?1.0f:1.0f-factor;
        Expect(Word(p,0)==(0.0f-g)*s+g && Bits(p,1)==0x3F800000 && Bits(p,2)==0 && Bits(p,3)==0,"cbGlowParam g "+std::to_string(g)+" factor "+std::to_string(factor));}
    {uint8_t p[16]{};glow::ParamImage(1.5f,0,1.0f,p);Expect(Word(p,0)==1.5f,"cbGlowParam factor 1 gives g exactly");}
    {uint8_t f[32]{};glow::FilterParamImage(320,180,f);
        Expect(Word(f,0)==5.0f && Word(f,1)==2.8125f && Word(f,2)==0 && Word(f,3)==0 && Word(f,4)==1.0f/320.0f && Word(f,5)==1.0f/180.0f,"cbFilterParam 320 x 180");}
    const glow::Size w720[4]{{320,180},{160,90},{80,45},{40,23}},w1080[4]{{480,270},{240,135},{120,68},{60,34}};
    for(unsigned l=0;l<4;++l){const auto a=glow::LevelSize(1280,720,l),b=glow::LevelSize(1920,1080,l);
        Expect(a.width==w720[l].width && a.height==w720[l].height,"level "+std::to_string(l)+" at 1280 x 720");
        Expect(b.width==w1080[l].width && b.height==w1080[l].height,"level "+std::to_string(l)+" at 1920 x 1080");}
    {const auto o=glow::LevelSize(1281,721,0);Expect(o.width==321 && o.height==181,"odd size rounds up");}
    std::printf("glow_math_test: %s (%d checks, %d failures)\n",failures?"FAILED":"passed",checks,failures);
    return failures?1:0;
}
