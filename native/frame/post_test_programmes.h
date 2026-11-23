// I-0138 tranche 4 (the final review's L3): the two sets of programmes the post stage tests draw a plan with (glare_stage_test,
// dof_stage_test, mb_stage_test), one copy. Header-only, for the offline tests.
#pragma once
#include "glare_library_block.h"
#include "../renderer/glare_stage.h"
#include "../renderer/post_glare_programs.h"
#include "../world/yebis_pins.h"
#include "../content/acquire.h"
#include <array>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace ds3native::post_test {
// The library's programmes of every technique of the plan (its draws and its pre draws), found in the executable by their pins
// (kept by technique: the image is scanned only for techniques not met yet). False, with its name, when a technique has no pin;
// throws when a pinned programme is not in the executable.
inline bool LibraryProgrammes(const std::vector<uint8_t>& exe,const yebis::GlarePlan& plan,GlarePrograms& out,std::string& unpinned){
    static GlarePrograms found;std::map<std::string,std::pair<std::string,std::string>> wanted;
    for(const auto* list:{&plan.draws,&plan.pre})for(const auto& g:*list){if(found.contains(g.technique))continue;const auto* pin=yebis::FindTechniquePin(g.technique.c_str());
        if(!pin){unpinned=g.technique;return false;}
        wanted[g.technique]={pin->vs_sha256,pin->ps_sha256};}
    if(!wanted.empty()){GlarePrograms fresh;
        for(size_t at=0;at+32<=exe.size();at+=4){
            if(std::memcmp(exe.data()+at,"DXBC",4))continue;uint32_t size{};std::memcpy(&size,exe.data()+at+24,4);
            if(size<32 || size>65536 || at+size>exe.size())continue;char hex[65]{};if(!ds3content::Sha256(exe.data()+at,size,hex))continue;
            for(const auto& [technique,shas]:wanted){if(shas.first==hex)fresh[technique].vertex.assign(exe.begin()+at,exe.begin()+at+size);
                if(shas.second==hex)fresh[technique].pixel.assign(exe.begin()+at,exe.begin()+at+size);}}
        for(const auto& [technique,shas]:wanted)if(fresh[technique].vertex.empty() || fresh[technique].pixel.empty())
            throw std::runtime_error("a pinned programme is not in the executable: "+technique);
        found.merge(fresh);}
    out.clear();for(const auto* list:{&plan.draws,&plan.pre})for(const auto& g:*list)out[g.technique]=found.at(g.technique);
    return true;}
// I-0138 tranche 8b: the stage tests' constant memory, over one mapped upload buffer (256-aligned ranges). The stage writes our
// block (yebis::glare_block_bytes) for every draw; for the library's programmes (`library`, the oracle) each block request takes
// the library's 4,096 bytes and Spread, called once the recording is made and before the list executes, rewrites every block in
// place into the library's registers (LibraryBlock). Other requests (the vertices) are given as asked.
// A block request is told by its size: never that of the vertices' requests (the step review's L5).
static_assert(yebis::glare_block_bytes!=GlareStage::vertex_request_bytes,"TestConstants tells a block from the vertices by its size");
class TestConstants {
public:
    TestConstants(uint8_t* cpu,D3D12_GPU_VIRTUAL_ADDRESS gpu,uint64_t capacity,bool library):cpu_(cpu),gpu_(gpu),capacity_(capacity),library_(library){}
    GlareConstantSink Sink(){return [this](uint64_t bytes){const bool block=library_ && bytes==yebis::glare_block_bytes;
        const uint64_t at=used_,size=((block?library_block_bytes:bytes)+255)/256*256;
        if(at+size>capacity_)throw std::runtime_error("the test's constant memory");
        used_+=size;if(block)blocks_.push_back(cpu_+at);return GlareConstantMemory{cpu_+at,gpu_+at};};}
    void Spread(){std::array<uint8_t,yebis::glare_block_bytes> ours{};
        for(auto* b:blocks_){std::memcpy(ours.data(),b,ours.size());LibraryBlock(ours.data(),b);}blocks_.clear();}
private:
    uint8_t* cpu_{};D3D12_GPU_VIRTUAL_ADDRESS gpu_{};uint64_t capacity_{},used_{};bool library_{};std::vector<uint8_t*> blocks_;
};
// The bytes a test's constant buffer needs for `draws` draws (a library block and a few vertices each).
inline uint64_t TestConstantBytes(size_t draws){return uint64_t(draws)*(library_block_bytes+512);}
// Our own programmes of every technique of the plan (post_glare_programs.h); throws, naming it, when a technique is not one of ours.
inline GlarePrograms OurProgrammes(const yebis::GlarePlan& plan){GlarePrograms mine;
    for(const auto* list:{&plan.draws,&plan.pre})for(const auto& g:*list){GlareBlobs blobs;
        if(!PostGlareProgram(g.technique,blobs))throw std::runtime_error("a technique is not one of our programmes: "+g.technique);
        mine[g.technique]=std::move(blobs);}
    return mine;}
}
