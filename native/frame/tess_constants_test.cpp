// W5e T-Q5/T-Q8/T-Q9 (WATER-TESS-WAVE1-READY 6): the GX04 parse and cbPNTriangles against the golden water-tess-blocks.tsv
// (argument: the file; the key mode/displacement/edge/inside/d7/fade start/fade end/d10/d11 is turned back into the GX04 data
// words), and cbTessScene for a synthetic camera (transpose, eye, -forward, unit inward planes).
#include "../renderer/tess_constants.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace ds3native;
namespace {
int failures{},checks{};
void Expect(bool ok,const std::string& what) {++checks;if(!ok){++failures;std::printf("FAIL %s\n",what.c_str());}}
std::vector<std::string> Split(const std::string& s,char c) {std::vector<std::string> v;std::stringstream ss(s);std::string x;while(std::getline(ss,x,c))v.push_back(x);return v;}
float Word(const uint8_t* p,size_t i) {float f;std::memcpy(&f,p+4*i,4);return f;}
uint32_t Bits(const uint8_t* p,size_t i) {uint32_t u;std::memcpy(&u,p+4*i,4);return u;}
}
int main(int argc,char** argv) {
    if(argc<2){std::printf("usage: tess_constants_test water-tess-blocks.tsv\n");return 2;}
    std::map<std::string,std::vector<uint32_t>> golden;std::vector<std::string> order;
    {std::ifstream in(argv[1]);std::string line;std::getline(in,line);
        while(std::getline(in,line)){if(!line.empty() && line.back()=='\r')line.pop_back();const auto c=Split(line,'\t');
            if(c.size()<4 || c[0]!="cbPNTriangles")continue;
            if(!golden.contains(c[1]))order.push_back(c[1]);golden[c[1]].push_back(static_cast<uint32_t>(std::stoul(c[3],nullptr,16)));}}
    Expect(golden.size()==8,"eight GX04 records in the golden");
    for(const auto& key:order){
        const auto v=Split(Split(key,' ')[0],'/');if(v.size()!=9){Expect(false,"key "+key);continue;}
        // the GX04 data words: d[3] displacement, d[4] mode, d[5..6] factors (floats), d[7] d7, d[8..9] fade, d[10] d10, d[11] d11.
        std::array<uint32_t,9> data{};data[0]=static_cast<uint32_t>(std::stoul(v[1]));data[1]=static_cast<uint32_t>(std::stoul(v[0]));
        const float floats[7]{std::stof(v[2]),std::stof(v[3]),std::stof(v[4]),std::stof(v[5]),std::stof(v[6]),std::stof(v[7]),std::stof(v[8])};
        for(size_t i=0;i<7;++i)std::memcpy(&data[2+i],&floats[i],4);
        tess::Material m;Expect(tess::FromGx04(reinterpret_cast<const uint8_t*>(data.data()),9,m),"parse "+key);
        uint8_t block[0x30]{};const bool built=tess::PnTriangles(m,1.0f,block);
        const auto& g=golden[key];Expect(built && g.size()>=10,"cbPNTriangles built "+key);
        for(size_t i=0;i<g.size() && i<10;++i)Expect(Bits(block,i)==g[i],"cbPNTriangles word "+std::to_string(i)+" of "+key);
        Expect(Bits(block,10)==0 && Bits(block,11)==0,"row 2 z, w zero");}
    {tess::Material none;uint8_t block[0x30]{};Expect(!tess::PnTriangles(none,1.0f,block),"no item: no block");
        const uint32_t gx90[7]{0x40E00000,0x40400000,0,0,0x41A00000,0x3F000000,0x3F800000};tess::Material g;
        Expect(tess::FromGx90(reinterpret_cast<const uint8_t*>(gx90),7,g) && g.mode==2 && g.displacement==1 && g.gx90 && g.edge==7 && g.inside==3,"GX90 fields");
        Expect(tess::PnTriangles(g,2.0f,block) && Word(block,8)==2.0f*1.0f,"GX90 displacement scale uses exparams+0x24");}
    // cbTessScene: a camera at (1, 2, 3) looking along +z (row-vector view = translate(-eye)), a perspective with y = 1.
    {const float z=1.0001f,n=0.1f;const std::array<float,16> view{1,0,0,0, 0,1,0,0, 0,0,1,0, -1,-2,-3,1},projection{1,0,0,0, 0,1,0,0, 0,0,z,1, 0,0,-z*n,0};
        std::array<float,16> vp{};for(size_t r=0;r<4;++r)for(size_t c=0;c<4;++c)for(size_t k=0;k<4;++k)vp[4*r+c]+=view[4*r+k]*projection[4*k+c];
        uint8_t block[0xB4]{};tess::TessScene(vp,{1,2,3},{0,0,2},5.5f,block);
        bool transposed=true;for(size_t r=0;r<4;++r)for(size_t c=0;c<4;++c)transposed=transposed && Word(block,4*r+c)==vp[4*c+r];
        Expect(transposed,"@0 transpose(view x projection)");
        Expect(Word(block,16)==1 && Word(block,17)==2 && Word(block,18)==3 && Word(block,19)==1,"@64 eye");
        Expect(Word(block,20)==0 && Word(block,21)==0 && Word(block,22)==-1 && Word(block,23)==0,"@80 -forward normalised");
        Expect(Word(block,24)==1 && Word(block,25)==0 && Word(block,26)==0 && Word(block,27)==0,"@96 (1, 0, 0, 0)");
        const float centre[4]{1,2,13,1};   // a point 10 ahead of the eye: inside every side plane
        for(size_t p=0;p<4;++p){float d{},norm{};for(size_t c=0;c<4;++c)d+=Word(block,28+4*p+c)*centre[c];for(size_t c=0;c<3;++c)norm+=Word(block,28+4*p+c)*Word(block,28+4*p+c);
            Expect(std::fabs(norm-1.0f)<1e-5f && d>0.0f,"plane "+std::to_string(p)+" unit and inward");}
        Expect(Word(block,44)==5.5f,"@176 clock");}
    // W7 TS-4b: the absolute-world view-projection from a camera-relative cbSceneParam image: for world points p, the block's
    // rows applied to (p, 1) equal the stored relative rows applied to (p - SC_CameraPos, 1), the domain against the ordinary VS.
    {uint8_t scene[960]{};const float eye[3]{120.5f,-33.25f,-410.0f};
        const float rel[16]{0.9f,0.1f,-0.2f,0.3f, -0.05f,1.4f,0.2f,-0.1f, 0.01f,0.02f,1.0001f,-0.1f, 0.02f,-0.03f,0.99f,0.0f};
        std::memcpy(scene+48,eye,12);std::memcpy(scene+384,rel,64);
        std::array<float,16> vp{};std::array<float,3> e{};tess::AbsoluteViewProjection(scene,vp,e);
        Expect(e[0]==eye[0] && e[1]==eye[1] && e[2]==eye[2],"eye is SC_CameraPos");
        uint8_t block[0xB4]{};tess::TessScene(vp,e,{0,0,1},0.0f,block);
        const float points[3][3]{{121.0f,-30.0f,-400.0f},{90.0f,-35.0f,-450.0f},{125.25f,-20.5f,-380.75f}};double worst{};
        for(const auto& p:points)for(size_t r=0;r<4;++r){double a{},b{};
            for(size_t k=0;k<3;++k){a+=double(Word(block,4*r+k))*p[k];b+=double(rel[4*r+k])*(double(p[k])-eye[k]);}
            a+=Word(block,4*r+3);b+=rel[4*r+3];worst=std::max(worst,std::fabs(a-b));}
        Expect(worst<1e-3,"absolute rows equal the relative ones on p - eye (worst "+std::to_string(worst)+")");}
    std::printf("tess_constants_test: %s (%d checks, %d failures)\n",failures?"FAILED":"passed",checks,failures);
    return failures?1:0;
}
