// I-0138 tranche 8b: our glare block (yebis::glare_block_registers) against the library's (228 float4, 4,096 bytes as the
// original uploads it), for the offline tests alone: the word-for-word tests read a library word from our block, the stage tests
// draw the library's programmes (the oracle) with our block spread to the library's registers, and post_draws_test draws ours
// with a captured block gathered into our slots. Header-only.
#pragma once
#include "../world/yebis_glare.h"
#include <cstring>

namespace ds3native::post_test {
inline constexpr uint32_t library_block_bytes=4096;
// The library's word `word` (register word / 4, component word % 4) as our block holds it: 0 for a register without a slot.
inline uint32_t LibraryWord(const yebis::GlareDraw& g,uint32_t word){
    const uint32_t reg=word/4;if(reg>=yebis::glare_library_registers || yebis::glare_block_slots[reg]<0)return 0;
    uint32_t v{};std::memcpy(&v,g.block.data()+16*size_t(yebis::glare_block_slots[reg])+4*(word%4),4);return v;}
// Our block (glare_block_bytes) spread into the library's (library_block_bytes, zero outside our registers). The two may not overlap.
inline void LibraryBlock(const uint8_t* ours,uint8_t* library){
    std::memset(library,0,library_block_bytes);
    for(size_t i=0;i<yebis::glare_block_registers.size();++i)std::memcpy(library+16*size_t(yebis::glare_block_registers[i]),ours+16*i,16);}
// A library block (library_block_bytes) gathered into ours (glare_block_bytes): the registers our programmes read.
inline void OurBlock(const uint8_t* library,uint8_t* ours){
    for(size_t i=0;i<yebis::glare_block_registers.size();++i)std::memcpy(ours+16*i,library+16*size_t(yebis::glare_block_registers[i]),16);}
}
