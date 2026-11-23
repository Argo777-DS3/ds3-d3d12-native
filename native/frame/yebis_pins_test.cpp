// W10 YB-2 T3 (YEBIS-WAVE1-READY 2, 6): every vertex and pixel blob of the world techniques (82 of YB-2, the glare, light-shaft, DoF and motion-blur rows added by hand, I-0108's downsample tap counts) is a DXBC container of the
// executable (the library's technique table holds them; each container is found by its magic and its size word, then
// hashed), the shared pixel blobs of TexCoordMadT1_S4 / MadT1_S4 and S2 hold, and CheckTechnique refuses a flipped byte.
// Argument: the executable (without it only the tap-count pins are checked).
#include "../world/yebis_pins.h"
#include "../content/acquire.h"
#include <cstdio>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include <vector>

int main(int argc,char** argv){
    if(argc!=2){std::printf("usage: yebis_pins_test <DarkSoulsIII.exe>\n");return 1;}
    int failures{};
    // I-0108: every tap count the DoF plan's plain-column downsample can name (nx x ny, 1..5 per axis) is pinned (no executable needed).
    for(int nx=1;nx<=5;++nx)for(int ny=1;ny<=5;++ny){const std::string technique="tech_MadT1_S"+std::to_string(nx*ny);
        if(!ds3native::yebis::FindTechniquePin(technique.c_str())){++failures;std::printf("FAIL %s not pinned\n",technique.c_str());}}
    std::ifstream file(argv[1],std::ios::binary);
    if(!file){if(failures){std::printf("FAIL yebis pins: %d failures\n",failures);return 1;}std::printf("PASS yebis pins: skipped, no executable (every downsample tap count pinned)\n");return 0;}
    const std::vector<uint8_t> exe((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    std::set<std::string> hashes;std::vector<std::pair<size_t,uint32_t>> containers;
    for(size_t at=0;at+32<=exe.size();++at){
        if(exe[at]!='D' || exe[at+1]!='X' || exe[at+2]!='B' || exe[at+3]!='C')continue;
        uint32_t size{};std::memcpy(&size,exe.data()+at+24,4);
        if(size<32 || size>(1u<<20) || at+size>exe.size())continue;
        char hex[65]{};if(!ds3content::Sha256(exe.data()+at,size,hex))continue;
        hashes.insert(hex);containers.push_back({at,size});}
    size_t observed{};
    for(const auto& pin:ds3native::yebis::world_techniques){
        const bool vs=hashes.count(pin.vs_sha256)!=0,ps=hashes.count(pin.ps_sha256)!=0;
        if(!vs || !ps){++failures;std::printf("FAIL %s: vertex %s, pixel %s\n",pin.technique,vs?"found":"missing",ps?"found":"missing");}
        if(pin.observed)++observed;}
    const auto* t4=ds3native::yebis::FindTechniquePin("tech_TexCoordMadT1_S4");const auto* m4=ds3native::yebis::FindTechniquePin("tech_MadT1_S4");
    const auto* t2=ds3native::yebis::FindTechniquePin("tech_TexCoordMadT1_S2");const auto* m2=ds3native::yebis::FindTechniquePin("tech_MadT1_S2");
    if(!t4 || !m4 || !t2 || !m2 || std::strcmp(t4->ps_sha256,m4->ps_sha256) || std::strcmp(t2->ps_sha256,m2->ps_sha256)){++failures;std::printf("FAIL the shared MadT1 pixel blobs\n");}
    // A flipped byte of a pinned pixel container is refused.
    {const auto& pin=ds3native::yebis::world_techniques[0];bool refused=false;
        for(const auto& [at,size]:containers){char hex[65]{};ds3content::Sha256(exe.data()+at,size,hex);if(std::strcmp(hex,pin.ps_sha256))continue;
            std::vector<uint8_t> copy(exe.begin()+at,exe.begin()+at+size);copy[size/2]^=1;char flipped[65]{};ds3content::Sha256(copy.data(),copy.size(),flipped);
            refused=!ds3native::yebis::CheckTechnique(pin.technique,pin.vs_sha256,flipped) && ds3native::yebis::CheckTechnique(pin.technique,pin.vs_sha256,hex);break;}
        if(!refused){++failures;std::printf("FAIL CheckTechnique on a flipped byte\n");}}
    if(failures){std::printf("FAIL yebis pins: %d failures\n",failures);return 1;}
    std::printf("PASS yebis pins: T3 the %zu world techniques (%zu observed) have their vertex and pixel containers in the executable (%zu containers scanned), the shared MadT1 pixel blobs, every downsample tap count pinned, a flipped byte refused\n",
        ds3native::yebis::world_techniques.size(),observed,containers.size());
    return 0;
}
