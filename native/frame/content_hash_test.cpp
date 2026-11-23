// I-0097 tranche 4 (D-0211): the content hash against XXH64's published values, and its sensitivity to every byte.
#include "../content/content_hash.h"
#include <cstdio>
#include <stdexcept>
#include <vector>

namespace {void Require(bool value,const char* message){if(!value)throw std::runtime_error(message);}}
int main(){try{
    const auto text=[](const char* s){return std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(s),std::strlen(s));};
    Require(ds3content::ContentHash64({})==0xEF46DB3751D8E999ull,"XXH64 of the empty input");
    Require(ds3content::ContentHash64(text("abc"))==0x44BC2CF5AD770999ull,"XXH64 of abc");
    // 39 bytes: the 32-byte stripes, then the 4-byte and 1-byte tails (python-xxhash's documented example).
    Require(ds3content::ContentHash64(text("Nobody inspects the spammish repetition"))==0xFBCEA83C8A378BF1ull,"XXH64 of a 39-byte input");
    // Every length through the 32-byte stripes and the 8, 4 and 1-byte tails, each byte changed once: a different hash.
    std::vector<uint8_t> bytes(200);for(size_t i=0;i<bytes.size();++i)bytes[i]=static_cast<uint8_t>(i*7+3);
    for(size_t length=1;length<=bytes.size();++length){const std::span<const uint8_t> view(bytes.data(),length);const auto base=ds3content::ContentHash64(view);
        Require(base==ds3content::ContentHash64(view),"deterministic");
        for(size_t i=0;i<length;++i){bytes[i]^=0x40;Require(ds3content::ContentHash64(view)!=base,"every byte counts");bytes[i]^=0x40;}
        if(length>1)Require(ds3content::ContentHash64(std::span<const uint8_t>(bytes.data(),length-1))!=base,"the length counts");}
    std::printf("PASS content hash: XXH64 known answers, every byte and the length of 1-200 byte inputs change it\n");
    return 0;
}catch(const std::exception& e){std::printf("FAIL %s\n",e.what());return 1;}}
