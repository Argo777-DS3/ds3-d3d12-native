// W5a SW-2 T-W1/T-W2 (SHADOWS-WAVE1-READY section 7): the shadow state and scene words against the golden
// shadow-world-blocks.tsv (evidence/I-0068/queries/shadow-ready, Get-ShadowWorldBlocks.ps1) for the 8 records and the atlas
// edges 2048/4096/8192, and the sizes for the five quality rows and the four classes.
#include "shadow_state.h"
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace ds3native;
namespace {
int failures{},checked{};
void Expect(bool condition,const std::string& what) {if(!condition){++failures;std::printf("FAIL %s\n",what.c_str());}}
uint32_t Bits(float f) {uint32_t u;std::memcpy(&u,&f,4);return u;}
}
int main(int argc,char** argv) {
    if(argc<2){std::printf("usage: shadow_state_test shadow-world-blocks.tsv\n");return 2;}
    std::ifstream in(argv[1]);if(!in){std::printf("FAIL cannot open %s\n",argv[1]);return 1;}
    // (source,id) -> R words; (source,id,edge) -> field name -> word.
    std::map<std::string,std::array<uint32_t,37>> records;
    std::map<std::string,std::map<std::string,uint32_t>> derived;
    std::string line;std::getline(in,line);
    while(std::getline(in,line)){
        if(!line.empty() && line.back()=='\r')line.pop_back();
        std::vector<std::string> c;std::stringstream ss(line);std::string cell;while(std::getline(ss,cell,'\t'))c.push_back(cell);
        if(c.size()<6)continue;
        const auto key=c[0]+"/"+c[1];const uint32_t word=static_cast<uint32_t>(std::stoul(c[5],nullptr,16));
        if(c[2]=="-"){const auto offset=std::stoul(c[3].substr(4),nullptr,16);records[key][offset/4]=word;}
        else derived[key+"/"+c[2]][c[4]]=word;
    }
    Expect(records.size()==8,"eight records in the golden");
    for(const auto& [key,words]:records){
        uint8_t r[0x94]{};std::memcpy(r,words.data(),sizeof r);
        const auto state=shadow::ShadowClamps(r,0.1f);
        for(const uint32_t edge:{2048u,4096u,8192u}){
            const auto found=derived.find(key+"/"+std::to_string(edge));
            if(found==derived.end()){Expect(false,key+" edge "+std::to_string(edge)+" missing");continue;}
            const auto& d=found->second;
            uint8_t block[0x260]{};shadow::SceneWords(r,state,edge,0,shadow::Constants{},block);
            const auto word=[&](uint32_t at){uint32_t u;std::memcpy(&u,block+at,4);return u;};
            const std::pair<const char*,uint32_t> expected[]{
                {"state+0xA70 near",Bits(state.near_z)},{"state+0xA7C split0",Bits(state.split0)},{"state+0xA80 split1",Bits(state.split1)},
                {"state+0xA84 split2",Bits(state.split2)},{"state+0xA74 fadeStart",Bits(state.fade_start)},{"state+0xA78 fadeDist",Bits(state.fade_dist)},
                {"block+0x1C0.x FC_ShadowMapParam.x (fadeEnd)",word(0x1C0)},{"block+0x1C0.y 1/max(fadeDist,tiny)",word(0x1C4)},
                {"block+0x1C0.z 0.5/edge",word(0x1C8)},{"block+0x1C0.w 0.5/edge",word(0x1CC)},
                {"block+0x240.x 1/edge",word(0x240)},{"block+0x240.y 1/edge",word(0x244)},{"block+0x240.z R+0x80",word(0x248)},
                {"block+0x250.x one",word(0x250)},{"block+0x250.y d0/den0",word(0x254)},{"block+0x250.z d0/den1",word(0x258)},{"block+0x250.w d0/den2",word(0x25C)},
                {"block+0x1D0.x ca*(1-cr)",word(0x1D0)},{"block+0x1D0.y ca*(1-cg)",word(0x1D4)},{"block+0x1D0.z ca*(1-cb)",word(0x1D8)},{"block+0x1D0.w R+0x68",word(0x1DC)},
                {"block+0x230.x DepthRate R+0x74",word(0x230)},{"block+0x230.y DepthScale R+0x78",word(0x234)},{"block+0x230.z KernelScale R+0x7C",word(0x238)}};
            for(const auto& [name,value]:expected){
                const auto golden=d.find(name);
                if(golden==d.end()){Expect(false,key+" "+name+" missing in the golden");continue;}
                ++checked;
                char what[160]{};std::snprintf(what,sizeof what,"%s edge %u %s: %08X against %08X",key.c_str(),edge,name,value,golden->second);
                // The cascade blend ratios (+0x250.y..w) of the golden were computed by PowerShell, whose [single] arithmetic runs
                // in double and rounds once at the end; the engine (FUN_14023A0C0's SSE code) and this code subtract and divide in
                // single precision, as native/world ShadowFieldsFromBanks does (its bank check compares with the engine's state object). Those words may differ by
                // one unit in the last place; every other word is exact.
                const bool ratio=std::strstr(name,"d0/den")!=nullptr;
                const uint32_t ulps=value>golden->second?value-golden->second:golden->second-value;
                Expect(ratio?ulps<=1:value==golden->second,what);
            }
        }
    }
    // T-W2: the world record's class 3 gives 1024, 1024, 2048, 4096, 8192 over the five rows, blur passes max(0, 1 + add).
    {const auto& r=records["mapbase/0"];uint8_t record[0x94]{};std::memcpy(record,r.data(),sizeof record);
        const uint32_t edges[5]{1024,1024,2048,4096,8192},passes[5]{0,0,0,1,3};
        for(size_t q=0;q<5;++q){const auto s=shadow::ShadowSizes(record,&shadow::quality_rows[q],false,2);
            Expect(s.width==edges[q] && s.height==edges[q] && s.blur_passes==passes[q],"world sizes at quality row "+std::to_string(q));}
        const auto base=shadow::ShadowSizes(record,nullptr,false,2);Expect(base.width==4096,"class 3 base edge without a row");
        const auto twice=shadow::ShadowSizes(record,&shadow::quality_rows[3],true,2);Expect(twice.width==4096,"double size clamped at HIGH's max");
        const auto twice_max=shadow::ShadowSizes(record,&shadow::quality_rows[4],true,2);Expect(twice_max.width==8192,"double size clamped at MAX's max");}
    Expect(shadow::ClassSize(0)==2048 &&shadow::ClassSize(1)==1024 && shadow::ClassSize(2)==2048 && shadow::ClassSize(3)==4096 && shadow::ClassSize(7)==2048,"class sizes");
    // SW-5 against row SL5 (shadow-sl5-oracle.tsv, Reference1500): the shadow words against the world cbSceneParam image.
    // I-0131 R10a: the cascades' atlas matrices, cascade words, viewports, rasterizer and filter plan, and their checks, are
    // removed with the cascades.
    int oracle_checked=0;double worst=0;
    if(argc>=3){
        std::ifstream oracle(argv[2]);if(!oracle){std::printf("FAIL cannot open %s\n",argv[2]);return 1;}
        std::vector<uint8_t> image;
        const auto bytes=[](const std::string& hex){std::vector<uint8_t> out(hex.size()/2);
            for(size_t i=0;i<out.size();++i)out[i]=static_cast<uint8_t>(std::stoul(hex.substr(2*i,2),nullptr,16));return out;};
        while(std::getline(oracle,line)){
            if(!line.empty() && line.back()=='\r')line.pop_back();
            if(line.empty() || line[0]=='#' || line.rfind("kind",0)==0)continue;
            std::vector<std::string> c;std::stringstream ss(line);std::string cell;while(std::getline(ss,cell,'\t'))c.push_back(cell);
            if(c.size()<3)continue;
            if(c[0]=="scene_image")image=bytes(c[2]);
        }
        Expect(image.size()==960,"oracle rows");
        if(image.size()==960){
            uint8_t words[960]{};uint8_t r[0x94]{};std::memcpy(r,records["mapbase/0"].data(),sizeof r);
            const auto st=shadow::ShadowClamps(r,0.1f);
            shadow::SceneWords(r,st,8192,32,shadow::Constants{},words);
            for(const uint32_t at:{0x1C0u,0x1D0u,0x230u,0x240u})Expect(std::memcmp(words+at,image.data()+at,16)==0,"image words +"+std::to_string(at));
            for(uint32_t at=0x250;at<0x260;at+=4){uint32_t v,e;std::memcpy(&v,words+at,4);std::memcpy(&e,image.data()+at,4);
                Expect((v>e?v-e:e-v)<=1,"image ratio word +"+std::to_string(at));}
        }
    }
    std::printf("shadow_state_test: %s (%d golden words, %d oracle words within %.2g, %d failures)\n",failures?"FAILED":"passed",checked,oracle_checked,worst,failures);
    return failures?1:0;
}
