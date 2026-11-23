#pragma once
#include <cstdint>
#include <cstring>
#include <span>

namespace ds3content {
// I-0097 tranche 4 (D-0211): a 64-bit content hash (XXH64, seed 0) of engine bytes, the key under which the native part of a
// streamed resource already prepared is found again when DS3 loads the same content (a reload of a map part). Reads every byte
// once (about the speed of a copy without its writes); no allocation.
inline uint64_t ContentHash64(std::span<const uint8_t> bytes,uint64_t seed=0) noexcept {
    constexpr uint64_t p1=11400714785074694791ull,p2=14029467366897019727ull,p3=1609587929392839161ull,
        p4=9650029242287828579ull,p5=2870177450012600261ull;
    const auto rotl=[](uint64_t x,int r){return (x<<r)|(x>>(64-r));};
    const auto word=[](const uint8_t* p){uint64_t v;std::memcpy(&v,p,8);return v;};
    const auto round=[&](uint64_t acc,uint64_t input){acc+=input*p2;acc=rotl(acc,31);return acc*p1;};
    const auto merge=[&](uint64_t acc,uint64_t value){acc^=round(0,value);return acc*p1+p4;};
    const uint8_t* p=bytes.data();const uint8_t* const end=p+bytes.size();uint64_t h;
    if(bytes.size()>=32){uint64_t v1=seed+p1+p2,v2=seed+p2,v3=seed,v4=seed-p1;
        for(;end-p>=32;p+=32){v1=round(v1,word(p));v2=round(v2,word(p+8));v3=round(v3,word(p+16));v4=round(v4,word(p+24));}
        h=rotl(v1,1)+rotl(v2,7)+rotl(v3,12)+rotl(v4,18);h=merge(h,v1);h=merge(h,v2);h=merge(h,v3);h=merge(h,v4);}
    else h=seed+p5;
    h+=static_cast<uint64_t>(bytes.size());
    for(;end-p>=8;p+=8){h^=round(0,word(p));h=rotl(h,27)*p1+p4;}   // final review: lengths, not pointers past the end
    if(end-p>=4){uint32_t v;std::memcpy(&v,p,4);h^=uint64_t(v)*p1;h=rotl(h,23)*p2+p3;p+=4;}
    for(;p<end;++p){h^=uint64_t(*p)*p5;h=rotl(h,11)*p1;}
    h^=h>>33;h*=p2;h^=h>>29;h*=p3;h^=h>>32;
    return h;
}
}
